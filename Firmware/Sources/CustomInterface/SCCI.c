// -----------------------------------------
// SCCI communication interface
// ----------------------------------------

// Header
#include "SCCI.h"
//
// Includes
#include "CRC16.h"
#include "SysConfig.h"
#include "BCCIMaster.h"
#include "StateMachine.h"
#include "DeviceProfile.h"
#include "DataTable.h"
#include "DeviceObjectDictionary.h"
#include "Global.h"
#include "Logger.h"

// Macro
//
#define START_BYTE				0xDBu
#define FUNCTION_CODE_MASK		(BIT3 | BIT4 | BIT5 | BIT6)
#define FUNCTION_SCODE_MASK		(BIT0 | BIT1 | BIT2)
#define RESPONSE_MASK			BIT7

// Constants
//
enum DispID
{
	DISP_NONE = 0,
	DISP_R_16 = 1,
	DISP_R_32 = 2,
	DISP_R_16_2 = 3,
	DISP_W_16 = 4,
	DISP_W_32 = 5,
	DISP_W_16_2 = 6,
	DISP_RB_16 = 7,
	DISP_RRB_16 = 8,
	DISP_WB_16 = 9,
	DISP_C = 10,
	DISP_RBF_16 = 11,
	DISP_RRBF_16 = 12,
	DISP_R_F = 15,
	DISP_W_F = 16,
	DISP_RBF_F = 17
};
//
enum SCCI_FunctionCodes
{
	FUNCTION_WRITE = 1,
	FUNCTION_READ = 2,
	FUNCTION_WRITE_BLOCK = 3,
	FUNCTION_READ_BLOCK = 4,
	FUNCTION_CALL = 5,
	FUNCTION_ERROR = 6,
	FUNCTION_FAST_READ_BLK = 7
};
//
enum SCCI_SubFunctionCodes
{
	SFUNC_16 = 1,
	SFUNC_32 = 2,
	SFUNC_16_2 = 3,
	SFUNC_REP_16 = 3,
	SFUNC_FLOAT = 5
};

// Forward functions
//
static void SCCI_DispatchHeader(pSCCI_Interface Interface);
static void SCCI_DispatchBody(pSCCI_Interface Interface, Boolean MaskStateChangeOperations);
static void SCCI_SendErrorFrame(pSCCI_Interface Interface, Int16U ErrorCode, Int16U Details);
static void SCCI_SendErrorFrameEx(pSCCI_Interface Interface, Int16U Node, Int16U ErrorCode, Int16U Details);
static void SCCI_SendResponseFrame(pSCCI_Interface Interface, Int16U FrameSize);
static void SCCI_SendResponseFrameEx(pSCCI_Interface Interface, Int16U Node, Int16U Function, Int16U SubFunction,
		Int16U FrameSize);
//
static void SCCI_HandleRead16(pSCCI_Interface Interface);
static void SCCI_HandleRead32(pSCCI_Interface Interface);
static void SCCI_HandleReadFloat(pSCCI_Interface Interface);
static void SCCI_HandleRead16Double(pSCCI_Interface Interface);
static void SCCI_HandleWrite16(pSCCI_Interface Interface);
static void SCCI_HandleWrite32(pSCCI_Interface Interface);
static void SCCI_HandleWriteFloat(pSCCI_Interface Interface);
static void SCCI_HandleWrite16Double(pSCCI_Interface Interface);
static void SCCI_HandleReadBlock16(pSCCI_Interface Interface, Boolean Repeat);
static void SCCI_HandleWriteBlock16(pSCCI_Interface Interface);
static void SCCI_HandleReadBlockFast16(pSCCI_Interface Interface, Boolean Repeat);
static void SCCI_HandleReadBlockFastFloat(pSCCI_Interface Interface);
static void SCCI_HandleCall(pSCCI_Interface Interface);

// Variables
//
static Int16U ZeroBuffer[xCCI_BUFFER_SIZE] = {0};

// Functions
//
void SCCI_Init(pSCCI_Interface Interface, pSCCI_IOConfig IOConfig, pxCCI_ServiceConfig ServiceConfig, pInt16U DataTable,
		Int16U DataTableSize, Int32U MessageTimeoutTicks, void *ArgumentForCallback)
{
	Int16U i;
	
	// Reset fields
	Interface->State = SCCI_STATE_WAIT_STARTBYTE;
	Interface->DispID = DISP_NONE;
	Interface->ExpectedBodyLength = 0;
	Interface->LastTimestampTicks = 0;
	Interface->ProtectionAndEndpoints.ProtectedAreasUsed = 0;
	
	for(i = 0; i < xCCI_BUFFER_SIZE; ++i)
		Interface->MessageBuffer[i] = 0;

	for(i = 0; i < xCCI_MAX_READ_ENDPOINTS; ++i)
	{
		Interface->ProtectionAndEndpoints.ReadEndpoints16[i].Callback = NULL;
		Interface->ProtectionAndEndpoints.ReadEndpoints16[i].Name = 0;
		Interface->ProtectionAndEndpoints.ReadEndpoints16[i].Initialized = FALSE;

		Interface->ProtectionAndEndpoints.ReadEndpoints32[i] = NULL;
	}

	for(i = 0; i < xCCI_MAX_WRITE_ENDPOINTS; ++i)
	{
		Interface->ProtectionAndEndpoints.WriteEndpoints16[i] = NULL;
		Interface->ProtectionAndEndpoints.WriteEndpoints32[i] = NULL;
	}

	// Save parameters
	Interface->IOConfig = IOConfig;
	Interface->ServiceConfig = ServiceConfig;
	Interface->DataTableAddress = DataTable;
	Interface->DataTableSize = DataTableSize;
	Interface->TimeoutValueTicks = MessageTimeoutTicks;
	Interface->ArgForEPCallback = ArgumentForCallback;
}
// ----------------------------------------

void SCCI_Process(pSCCI_Interface Interface, Int64U CurrentTickCount, Boolean MaskStateChangeOperations)
{
	switch(Interface->State)
	{
		case SCCI_STATE_WAIT_STARTBYTE:
			while(Interface->IOConfig->IO_GetBytesToReceive())
			{
				Int16U startByte = Interface->IOConfig->IO_ReceiveByte();
				LOG_SaveValues(&startByte, 1);

				if(startByte == START_BYTE)
				{
					Interface->State = SCCI_STATE_WAIT_NID;
					Interface->LastTimestampTicks = CurrentTickCount + Interface->TimeoutValueTicks;

					if(Interface->IOConfig->IO_GetBytesToReceive())
						goto GOTO_PROCESS_NID;
					else
						break;
				}
			}
			break;

		case SCCI_STATE_WAIT_NID:
			if(Interface->IOConfig->IO_GetBytesToReceive())
			{
				GOTO_PROCESS_NID:;

				Int16U NodeID = Interface->IOConfig->IO_ReceiveByte();
				LOG_SaveValues(&NodeID, 1);

				Interface->MessageBuffer[0] = NodeID | (START_BYTE << 8);
				Interface->State = SCCI_STATE_WAIT_HEADER;

				if(Interface->IOConfig->IO_GetBytesToReceive() >= 2)
					goto GOTO_PROCESS_HEADER;
			}
			break;

		case SCCI_STATE_WAIT_HEADER:
			if(Interface->IOConfig->IO_GetBytesToReceive() >= 2)
			{
				GOTO_PROCESS_HEADER:;

				Interface->IOConfig->IO_ReceiveArray16(Interface->MessageBuffer + 1, 1);
				LOG_SaveValues(Interface->MessageBuffer + 1, 1);
				SCCI_DispatchHeader(Interface);

				if(Interface->State == SCCI_STATE_WAIT_BODY
						&& Interface->IOConfig->IO_GetBytesToReceive() >= Interface->ExpectedBodyLength * 2)
					goto GOTO_PROCESS_BODY;
			}
			break;

		case SCCI_STATE_WAIT_BODY:
			if(Interface->IOConfig->IO_GetBytesToReceive() >= Interface->ExpectedBodyLength * 2)
			{
				GOTO_PROCESS_BODY:;

				Interface->IOConfig->IO_ReceiveArray16(Interface->MessageBuffer + 2, Interface->ExpectedBodyLength);
				LOG_SaveValues(Interface->MessageBuffer + 2, Interface->ExpectedBodyLength);
				SCCI_DispatchBody(Interface, MaskStateChangeOperations);
			}
			break;
	}

	if(Interface->TimeoutValueTicks && Interface->State != SCCI_STATE_WAIT_STARTBYTE
			&& CurrentTickCount > Interface->LastTimestampTicks)
	{
		SCCI_SendErrorFrame(Interface, ERR_TIMEOUT, Interface->State);
		Interface->State = SCCI_STATE_WAIT_STARTBYTE;
	}
}
// ----------------------------------------

void SCCI_AnswerRead16(pSCCI_Interface Interface, Int16U Node, Int16U Address, Int16U Value)
{
	Interface->MessageBuffer[2] = Address;
	Interface->MessageBuffer[3] = Value;

	SCCI_SendResponseFrameEx(Interface, Node, FUNCTION_READ, SFUNC_16, 5);
}
// ----------------------------------------

void SCCI_AnswerRead16Double(pSCCI_Interface Interface, Int16U Node, Int16U Address1, Int16U Value1, Int16U Address2,
		Int16U Value2)
{
	Interface->MessageBuffer[2] = Address1;
	Interface->MessageBuffer[3] = Value1;
	Interface->MessageBuffer[4] = Address2;
	Interface->MessageBuffer[5] = Value2;

	SCCI_SendResponseFrameEx(Interface, Node, FUNCTION_READ, SFUNC_16_2, 7);
}
// ----------------------------------------

void SCCI_AnswerRead32(pSCCI_Interface Interface, Int16U Node, Int16U Address, Int32U Value)
{
	Interface->MessageBuffer[2] = Address;
	Interface->MessageBuffer[3] = Value >> 16;
	Interface->MessageBuffer[4] = Value & 0xFFFF;

	SCCI_SendResponseFrameEx(Interface, Node, FUNCTION_READ, SFUNC_32, 6);
}
// ----------------------------------------

void SCCI_AnswerReadFloat(pSCCI_Interface Interface, Int16U Node, Int16U Address, Int32U Value)
{
	Interface->MessageBuffer[2] = Address;
	Interface->MessageBuffer[3] = Value >> 16;
	Interface->MessageBuffer[4] = Value & 0x0000FFFF;

	SCCI_SendResponseFrameEx(Interface, Node, FUNCTION_READ, SFUNC_FLOAT, 6);
}
// ----------------------------------------

void SCCI_AnswerWrite16(pSCCI_Interface Interface, Int16U Node, Int16U Address)
{
	Interface->MessageBuffer[2] = Address;

	SCCI_SendResponseFrameEx(Interface, Node, FUNCTION_WRITE, SFUNC_16, 4);
}
// ----------------------------------------

void SCCI_AnswerWrite16Double(pSCCI_Interface Interface, Int16U Node, Int16U Address1, Int16U Address2)
{
	Interface->MessageBuffer[2] = Address1;
	Interface->MessageBuffer[3] = Address2;

	SCCI_SendResponseFrameEx(Interface, Node, FUNCTION_WRITE, SFUNC_16_2, 5);
}
// ----------------------------------------

void SCCI_AnswerWriteBlock16(pSCCI_Interface Interface, Int16U Node, Int16U Endpoint)
{
	Interface->MessageBuffer[2] = Endpoint;

	SCCI_SendResponseFrameEx(Interface, Node, FUNCTION_WRITE_BLOCK, SFUNC_16, 4);
}
// ----------------------------------------

void SCCI_AnswerWrite32(pSCCI_Interface Interface, Int16U Node, Int16U Address)
{
	Interface->MessageBuffer[2] = Address;

	SCCI_SendResponseFrameEx(Interface, Node, FUNCTION_WRITE, SFUNC_32, 4);
}
// ----------------------------------------

void SCCI_AnswerWriteFloat(pSCCI_Interface Interface, Int16U Node, Int16U Address)
{
	Interface->MessageBuffer[2] = Address;

	SCCI_SendResponseFrameEx(Interface, Node, FUNCTION_WRITE, SFUNC_FLOAT, 4);
}
// ----------------------------------------

void SCCI_AnswerCall(pSCCI_Interface Interface, Int16U Node, Int16U Action)
{
	Interface->MessageBuffer[2] = Action;

	SCCI_SendResponseFrameEx(Interface, Node, FUNCTION_CALL, 0, 4);
}
// ----------------------------------------

void SCCI_AnswerReadBlock16Fast(pSCCI_Interface Interface, Int16U Node, Int16U Endpoint, pInt16U Data, Int16U DataSize)
{
	Interface->MessageBuffer[2] = (Endpoint << 8) | (SCCI_USE_CRC_IN_STREAM ? 1 : 0);

	if(!DataSize || (DataSize > xCCI_BLOCK_STM_MAX_VAL))
		DataSize = 0;

	Interface->MessageBuffer[3] = DataSize;

	if(SCCI_USE_CRC_IN_STREAM)
		Interface->MessageBuffer[4] = CRC16_ComputeCRC(Data, DataSize);

	SCCI_SendResponseFrameEx(Interface, Node, FUNCTION_FAST_READ_BLK, SFUNC_16, 6);

	Interface->IOConfig->IO_SendArray16(Data, DataSize);
	Interface->IOConfig->IO_SendArray16(ZeroBuffer, (8 - DataSize % 8) % 8);
}
// ----------------------------------------

void SCCI_AnswerReadBlockFloatFast(pSCCI_Interface Interface, Int16U Node, Int16U Endpoint, pInt32U Data, Int16U DataSize)
{
	Interface->MessageBuffer[2] = (Endpoint << 8) | (SCCI_USE_CRC_IN_STREAM ? 1 : 0);

	DataSize *= 2;
	pInt16U ShortData = (pInt16U)Data;

	if(DataSize > xCCI_BLOCK_STM_MAX_VAL)
		DataSize = 0;

	Interface->MessageBuffer[3] = DataSize;

	if(SCCI_USE_CRC_IN_STREAM)
		Interface->MessageBuffer[4] = CRC16_ComputeCRC(ShortData, DataSize);

	SCCI_SendResponseFrameEx(Interface, Node, FUNCTION_FAST_READ_BLK, SFUNC_FLOAT, 6);

	Interface->IOConfig->IO_SendArray16(ShortData, DataSize);
	Interface->IOConfig->IO_SendArray16((pInt16U)ZeroBuffer, (8 - DataSize % 8) % 8);
}
// ----------------------------------------

void SCCI_AnswerError(pSCCI_Interface Interface, Int16U Node, Int16U ErrorCode, Int16U Details)
{
	SCCI_SendErrorFrameEx(Interface, Node, ErrorCode, Details);
}
// ----------------------------------------

static void SCCI_DispatchHeader(pSCCI_Interface Interface)
{
	if(((Interface->MessageBuffer[0] >> 8) == START_BYTE))
	{
		Int16U fnc = Interface->MessageBuffer[1] >> 8;
		switch ((fnc & FUNCTION_CODE_MASK) >> 3)
		{
			case FUNCTION_WRITE:
				switch (fnc & FUNCTION_SCODE_MASK)
				{
					case SFUNC_16:
						Interface->ExpectedBodyLength = 3;
						Interface->State = SCCI_STATE_WAIT_BODY;
						Interface->DispID = DISP_W_16;
						break;
					case SFUNC_32:
						Interface->ExpectedBodyLength = 4;
						Interface->State = SCCI_STATE_WAIT_BODY;
						Interface->DispID = DISP_W_32;
						break;
					case SFUNC_16_2:
						Interface->ExpectedBodyLength = 5;
						Interface->State = SCCI_STATE_WAIT_BODY;
						Interface->DispID = DISP_W_16_2;
						break;
					case SFUNC_FLOAT:
						Interface->ExpectedBodyLength = 4;
						Interface->State = SCCI_STATE_WAIT_BODY;
						Interface->DispID = DISP_W_F;
						break;
					default:
						SCCI_SendErrorFrame(Interface, ERR_INVALID_SFUNCTION, fnc & FUNCTION_SCODE_MASK);
						break;
				}
				break;
			case FUNCTION_READ:
				switch (fnc & FUNCTION_SCODE_MASK)
				{
					case SFUNC_16:
						Interface->ExpectedBodyLength = 2;
						Interface->State = SCCI_STATE_WAIT_BODY;
						Interface->DispID = DISP_R_16;
						break;
					case SFUNC_32:
						Interface->ExpectedBodyLength = 2;
						Interface->State = SCCI_STATE_WAIT_BODY;
						Interface->DispID = DISP_R_32;
						break;
					case SFUNC_16_2:
						Interface->ExpectedBodyLength = 3;
						Interface->State = SCCI_STATE_WAIT_BODY;
						Interface->DispID = DISP_R_16_2;
						break;
					case SFUNC_FLOAT:
						Interface->ExpectedBodyLength = 2;
						Interface->State = SCCI_STATE_WAIT_BODY;
						Interface->DispID = DISP_R_F;
						break;
					default:
						SCCI_SendErrorFrame(Interface, ERR_INVALID_SFUNCTION, fnc & FUNCTION_SCODE_MASK);
						break;
				}
				break;
			case FUNCTION_WRITE_BLOCK:
				switch (fnc & FUNCTION_SCODE_MASK)
				{
					case SFUNC_16:
						Interface->ExpectedBodyLength = 5;
						Interface->State = SCCI_STATE_WAIT_BODY;
						Interface->DispID = DISP_WB_16;
						break;
					case SFUNC_32:
						SCCI_SendErrorFrame(Interface, ERR_NOT_SUPPORTED, fnc & FUNCTION_SCODE_MASK);
						break;
					default:
						SCCI_SendErrorFrame(Interface, ERR_INVALID_SFUNCTION, fnc & FUNCTION_SCODE_MASK);
						break;
				}
				break;
			case FUNCTION_READ_BLOCK:
				switch (fnc & FUNCTION_SCODE_MASK)
				{
					case SFUNC_16:
						Interface->ExpectedBodyLength = 2;
						Interface->State = SCCI_STATE_WAIT_BODY;
						Interface->DispID = DISP_RB_16;
						break;
					case SFUNC_32:
						SCCI_SendErrorFrame(Interface, ERR_NOT_SUPPORTED, fnc & FUNCTION_SCODE_MASK);
						break;
					case SFUNC_REP_16:
						Interface->ExpectedBodyLength = 2;
						Interface->State = SCCI_STATE_WAIT_BODY;
						Interface->DispID = DISP_RRB_16;
						break;
					case SFUNC_FLOAT:
						Interface->ExpectedBodyLength = 2;
						Interface->State = SCCI_STATE_WAIT_BODY;
						Interface->DispID = DISP_RBF_F;
						break;
					default:
						SCCI_SendErrorFrame(Interface, ERR_INVALID_SFUNCTION, fnc & FUNCTION_SCODE_MASK);
						break;
				}
				break;
			case FUNCTION_CALL:
				if((fnc & FUNCTION_SCODE_MASK) == 0)
				{
					Interface->ExpectedBodyLength = 2;
					Interface->State = SCCI_STATE_WAIT_BODY;
					Interface->DispID = DISP_C;
				}
				else
					SCCI_SendErrorFrame(Interface, ERR_INVALID_SFUNCTION, fnc & FUNCTION_SCODE_MASK);
				break;
			case FUNCTION_FAST_READ_BLK:
				switch (fnc & FUNCTION_SCODE_MASK)
				{
					case SFUNC_16:
						Interface->ExpectedBodyLength = 2;
						Interface->State = SCCI_STATE_WAIT_BODY;
						Interface->DispID = DISP_RBF_16;
						break;
					case SFUNC_32:
						SCCI_SendErrorFrame(Interface, ERR_NOT_SUPPORTED, fnc & FUNCTION_SCODE_MASK);
						break;
					case SFUNC_REP_16:
						Interface->ExpectedBodyLength = 2;
						Interface->State = SCCI_STATE_WAIT_BODY;
						Interface->DispID = DISP_RRBF_16;
						break;
					default:
						SCCI_SendErrorFrame(Interface, ERR_INVALID_SFUNCTION, fnc & FUNCTION_SCODE_MASK);
						break;
				}
				break;
			default:
				SCCI_SendErrorFrame(Interface, ERR_INVALID_FUNCTION, fnc & FUNCTION_CODE_MASK);
				break;
		}
	}
}
// ----------------------------------------

static void SCCI_DispatchBody(pSCCI_Interface Interface, Boolean MaskStateChangeOperations)
{
	Int16U crc = CRC16_ComputeCRC(Interface->MessageBuffer, Interface->ExpectedBodyLength + 1);
	if(Interface->MessageBuffer[Interface->ExpectedBodyLength + 1] != crc)
	{
		SCCI_SendErrorFrame(Interface, ERR_CRC, crc);
		return;
	}

	if(MaskStateChangeOperations && (Interface->DispID > DISP_R_16_2))
	{
		SCCI_SendErrorFrame(Interface, ERR_BLOCKED, 0);
		return;
	}

	Int16U node, func, addr;
	node = Interface->MessageBuffer[0] & 0xFF;
	func = Interface->DispID;
	addr = Interface->MessageBuffer[2];

	if(!(node == 0 && func == DISP_R_16 && REG_DIAG_RS232_NODE <= addr && addr <= REG_DIAG_RS232_ADDRESS))
	{
		DataTable[REG_DIAG_RS232_NODE] = node;
		DataTable[REG_DIAG_RS232_FUNC_CODE] = func;
		DataTable[REG_DIAG_RS232_ADDRESS] = addr;
	}

	switch (Interface->DispID)
	{
		case DISP_R_16:
			SCCI_HandleRead16(Interface);
			break;
		case DISP_R_32:
			SCCI_HandleRead32(Interface);
			break;
		case DISP_R_16_2:
			SCCI_HandleRead16Double(Interface);
			break;
		case DISP_W_16:
			SCCI_HandleWrite16(Interface);
			break;
		case DISP_W_32:
			SCCI_HandleWrite32(Interface);
			break;
		case DISP_W_16_2:
			SCCI_HandleWrite16Double(Interface);
			break;
		case DISP_RB_16:
			SCCI_HandleReadBlock16(Interface, FALSE);
			break;
		case DISP_RRB_16:
			SCCI_HandleReadBlock16(Interface, TRUE);
			break;
		case DISP_R_F:
			SCCI_HandleReadFloat(Interface);
			break;
		case DISP_W_F:
			SCCI_HandleWriteFloat(Interface);
			break;
		case DISP_WB_16:
			SCCI_HandleWriteBlock16(Interface);
			break;
		case DISP_C:
			SCCI_HandleCall(Interface);
			break;
		case DISP_RBF_16:
			SCCI_HandleReadBlockFast16(Interface, FALSE);
			break;
		case DISP_RRBF_16:
			SCCI_HandleReadBlockFast16(Interface, TRUE);
			break;
		case DISP_RBF_F:
			SCCI_HandleReadBlockFastFloat(Interface);
			break;
		default:
			SCCI_SendErrorFrame(Interface, ERR_NOT_SUPPORTED, 0);
			break;
	}
}
// ----------------------------------------

static void SCCI_HandleRead16(pSCCI_Interface Interface)
{
	Int16U node = Interface->MessageBuffer[0] & 0xFF;
	Int16U addr = Interface->MessageBuffer[2];

	if(node == DEVICE_SCCI_ADDRESS)
	{
		if(addr >= Interface->DataTableSize)
		{
			SCCI_SendErrorFrame(Interface, ERR_INVALID_ADDESS, addr);
		}
		else
		{
			Interface->MessageBuffer[3] = Interface->DataTableAddress[addr];
			SCCI_SendResponseFrame(Interface, 5);
		}
	}
	else
	{
		Int16U data;
		Int16U err = BCCIM_Read16(&DEVICE_CAN_Interface, node, addr, &data);

		if(err == ERR_NO_ERROR)
			SCCI_AnswerRead16(Interface, node, addr, data);
		else
			SCCI_AnswerError(Interface, node, err, BCCIM_GetSavedErrorDetails());
	}
}
// ----------------------------------------

static void SCCI_HandleRead16Double(pSCCI_Interface Interface)
{
	Int16U node = Interface->MessageBuffer[0] & 0xFF;
	Int16U addr1 = Interface->MessageBuffer[2];
	Int16U addr2 = Interface->MessageBuffer[3];

	if(node == DEVICE_SCCI_ADDRESS)
	{
		if(addr1 >= Interface->DataTableSize)
		{
			SCCI_SendErrorFrame(Interface, ERR_INVALID_ADDESS, addr1);
		}
		else
		{
			Interface->MessageBuffer[3] = Interface->DataTableAddress[addr1];

			if(addr2 >= Interface->DataTableSize)
			{
				SCCI_SendErrorFrame(Interface, ERR_INVALID_ADDESS, addr2);
			}
			else
			{
				Interface->MessageBuffer[4] = addr2;
				Interface->MessageBuffer[5] = Interface->DataTableAddress[addr2];

				SCCI_SendResponseFrame(Interface, 7);
			}
		}
	}
	else
	{
		SCCI_SendErrorFrame(Interface, ERR_NOT_SUPPORTED, (Interface->MessageBuffer[1] >> 8) & FUNCTION_SCODE_MASK);
	}
}

// ----------------------------------------

static void SCCI_HandleRead32(pSCCI_Interface Interface)
{
	Int16U node = Interface->MessageBuffer[0] & 0xFF;
	Int16U addr = Interface->MessageBuffer[2];

	if(node == DEVICE_SCCI_ADDRESS)
	{
		if((addr + 1) >= Interface->DataTableSize)
		{
			SCCI_SendErrorFrame(Interface, ERR_INVALID_ADDESS, addr + 1);
		}
		else
		{
			Int32U data = Interface->ServiceConfig->Read32Service(Interface->DataTableAddress, addr);

			Interface->MessageBuffer[3] = data >> 16;
			Interface->MessageBuffer[4] = data & 0x0000FFFF;

			SCCI_SendResponseFrame(Interface, 6);
		}
	}
	else
	{
		Int32U data;
		Int16U err = BCCIM_Read32(&DEVICE_CAN_Interface, node, addr, &data);

		if(err == ERR_NO_ERROR)
			SCCI_AnswerRead32(Interface, node, addr, data);
		else
			SCCI_AnswerError(Interface, node, err, BCCIM_GetSavedErrorDetails());
	}
}
// ----------------------------------------

static void SCCI_HandleReadFloat(pSCCI_Interface Interface)
{
	Int16U node = Interface->MessageBuffer[0] & 0xFF;
	Int16U addr = Interface->MessageBuffer[2];

	if(node == DEVICE_SCCI_ADDRESS)
	{
		if((addr + 1) >= Interface->DataTableSize)
		{
			SCCI_SendErrorFrame(Interface, ERR_INVALID_ADDESS, addr + 1);
		}
		else
		{
			Int32U data = Interface->ServiceConfig->Read32Service(Interface->DataTableAddress, addr);

			Interface->MessageBuffer[3] = data >> 16;
			Interface->MessageBuffer[4] = data & 0x0000FFFF;

			SCCI_SendResponseFrame(Interface, 6);
		}
	}
	else
	{
		Int32U data;
		Int16U err = BCCIM_ReadFloat(&DEVICE_CAN_Interface, node, addr, &data);

		if(err == ERR_NO_ERROR)
			SCCI_AnswerReadFloat(Interface, node, addr, data);
		else
			SCCI_AnswerError(Interface, node, err, BCCIM_GetSavedErrorDetails());
	}
}
// ----------------------------------------

static void SCCI_HandleWrite16(pSCCI_Interface Interface)
{
	Int16U node = Interface->MessageBuffer[0] & 0xFF;
	Int16U addr = Interface->MessageBuffer[2];
	Int16U data = Interface->MessageBuffer[3];

	if(node == DEVICE_SCCI_ADDRESS)
	{

		if(addr >= Interface->DataTableSize)
		{
			SCCI_SendErrorFrame(Interface, ERR_INVALID_ADDESS, addr);
		}
		else if(xCCI_InProtectedZone(&Interface->ProtectionAndEndpoints, addr))
		{
			SCCI_SendErrorFrame(Interface, ERR_PROTECTED, addr);
		}
		else if(Interface->ServiceConfig->ValidateCallback16
				&& !Interface->ServiceConfig->ValidateCallback16(addr, data))
		{
			SCCI_SendErrorFrame(Interface, ERR_VALIDATION, addr);
		}
		else
		{
			Interface->DataTableAddress[addr] = data;
			SCCI_SendResponseFrame(Interface, 4);
		}
	}
	else
	{
		Int16U err = BCCIM_Write16(&DEVICE_CAN_Interface, node, addr, data);

		if(err == ERR_NO_ERROR)
			SCCI_AnswerWrite16(Interface, node, addr);
		else
			SCCI_AnswerError(Interface, node, err, BCCIM_GetSavedErrorDetails());
	}
}
// ----------------------------------------

static void SCCI_HandleWrite32(pSCCI_Interface Interface)
{
	Int16U node = Interface->MessageBuffer[0] & 0xFF;
	Int16U addr = Interface->MessageBuffer[2];
	Int32U data = (((Int32U)(Interface->MessageBuffer[3])) << 16) | Interface->MessageBuffer[4];

	if(node == DEVICE_SCCI_ADDRESS)
	{
		if((addr + 1) >= Interface->DataTableSize)
		{
			SCCI_SendErrorFrame(Interface, ERR_INVALID_ADDESS, addr + 1);
		}
		else if(xCCI_InProtectedZone(&Interface->ProtectionAndEndpoints, addr)
				|| xCCI_InProtectedZone(&Interface->ProtectionAndEndpoints, addr + 1))
		{
			SCCI_SendErrorFrame(Interface, ERR_PROTECTED, addr);
		}
		else if(Interface->ServiceConfig->ValidateCallback32
				&& !Interface->ServiceConfig->ValidateCallback32(addr, data))
		{
			SCCI_SendErrorFrame(Interface, ERR_VALIDATION, addr);
		}
		else
		{
			Interface->ServiceConfig->Write32Service(Interface->DataTableAddress, addr, data);
			SCCI_SendResponseFrame(Interface, 4);
		}
	}
	else
	{
		Int16U err = BCCIM_Write32(&DEVICE_CAN_Interface, node, addr, data);

		if(err == ERR_NO_ERROR)
			SCCI_AnswerWrite32(Interface, node, addr);
		else
			SCCI_AnswerError(Interface, node, err, BCCIM_GetSavedErrorDetails());
	}
}
// ----------------------------------------

static void SCCI_HandleWriteFloat(pSCCI_Interface Interface)
{
	Int16U node = Interface->MessageBuffer[0] & 0xFF;
	Int16U addr = Interface->MessageBuffer[2];
	Int32U data = (((Int32U)(Interface->MessageBuffer[3])) << 16) | Interface->MessageBuffer[4];

	if(node == DEVICE_SCCI_ADDRESS)
	{
		if((addr + 1) >= Interface->DataTableSize)
		{
			SCCI_SendErrorFrame(Interface, ERR_INVALID_ADDESS, addr + 1);
		}
		else if(xCCI_InProtectedZone(&Interface->ProtectionAndEndpoints, addr)
				|| xCCI_InProtectedZone(&Interface->ProtectionAndEndpoints, addr + 1))
		{
			SCCI_SendErrorFrame(Interface, ERR_PROTECTED, addr);
		}
		else if(Interface->ServiceConfig->ValidateCallback32
				&& !Interface->ServiceConfig->ValidateCallback32(addr, data))
		{
			SCCI_SendErrorFrame(Interface, ERR_VALIDATION, addr);
		}
		else
		{
			Interface->ServiceConfig->Write32Service(Interface->DataTableAddress, addr, data);
			SCCI_SendResponseFrame(Interface, 4);
		}
	}
	else
	{
		Int16U err = BCCIM_WriteFloat(&DEVICE_CAN_Interface, node, addr, data);

		if(err == ERR_NO_ERROR)
			SCCI_AnswerWriteFloat(Interface, node, addr);
		else
			SCCI_AnswerError(Interface, node, err, BCCIM_GetSavedErrorDetails());
	}
}

// ----------------------------------------

static void SCCI_HandleWrite16Double(pSCCI_Interface Interface)
{
	Int16U node = Interface->MessageBuffer[0] & 0xFF;
	Int16U addr1 = Interface->MessageBuffer[2];
	Int16U addr2 = Interface->MessageBuffer[4];
	Int16U data1 = Interface->MessageBuffer[3];
	Int16U data2 = Interface->MessageBuffer[5];

	if(node == DEVICE_SCCI_ADDRESS)
	{
		if(addr1 >= Interface->DataTableSize)
		{
			SCCI_SendErrorFrame(Interface, ERR_INVALID_ADDESS, addr1);
		}
		else if(xCCI_InProtectedZone(&Interface->ProtectionAndEndpoints, addr1))
		{
			SCCI_SendErrorFrame(Interface, ERR_PROTECTED, addr1);
		}
		else if(Interface->ServiceConfig->ValidateCallback16
				&& !Interface->ServiceConfig->ValidateCallback16(addr1, data1))
		{
			SCCI_SendErrorFrame(Interface, ERR_VALIDATION, addr1);
		}
		else
		{
			if(addr2 >= Interface->DataTableSize)
			{
				SCCI_SendErrorFrame(Interface, ERR_INVALID_ADDESS, addr2);
			}
			else if(xCCI_InProtectedZone(&Interface->ProtectionAndEndpoints, addr2))
			{
				SCCI_SendErrorFrame(Interface, ERR_PROTECTED, addr2);
			}
			else if(Interface->ServiceConfig->ValidateCallback16
					&& !Interface->ServiceConfig->ValidateCallback16(addr2, data2))
			{
				SCCI_SendErrorFrame(Interface, ERR_VALIDATION, addr2);
			}
			else
			{
				Interface->DataTableAddress[addr1] = data1;
				Interface->DataTableAddress[addr2] = data2;
				Interface->MessageBuffer[3] = addr2;

				SCCI_SendResponseFrame(Interface, 5);
			}
		}
	}
	else
	{
		SCCI_SendErrorFrame(Interface, ERR_NOT_SUPPORTED, (Interface->MessageBuffer[1] >> 8) & FUNCTION_SCODE_MASK);
	}
}
// ----------------------------------------

static void SCCI_HandleReadBlock16(pSCCI_Interface Interface, Boolean Repeat)
{
	pInt16U src;
	Int16U length;
	Int16U node = Interface->MessageBuffer[0] & 0xFF;
	Int16U epnt = Interface->MessageBuffer[2] >> 8;
	Int16U epnt_index;

	if(node == DEVICE_SCCI_ADDRESS)
	{
		if(xCCI_EndpointIndex(&Interface->ProtectionAndEndpoints, epnt, &epnt_index))
		{
			xCCI_FUNC_CallbackReadEndpoint16 Callback =
					(xCCI_FUNC_CallbackReadEndpoint16)Interface->ProtectionAndEndpoints.ReadEndpoints16[epnt_index].Callback;
			length = Callback(epnt_index + 1, &src, FALSE, Repeat, Interface->ArgForEPCallback, SCCI_BLOCK_MAX_VAL_16_R);

			MemZero16(&Interface->MessageBuffer[3], SCCI_BLOCK_MAX_VAL_16_R);

			if(!length || (length > SCCI_BLOCK_MAX_VAL_16_R))
			{
				Interface->MessageBuffer[2] &= 0xFF00;
			}
			else
			{
				Interface->MessageBuffer[2] = (epnt << 8) | length;
				MemCopy16(src, &Interface->MessageBuffer[3], length);
			}

			SCCI_SendResponseFrame(Interface, SCCI_BLOCK_MAX_VAL_16_R + 4);
		}
		else
			SCCI_SendErrorFrame(Interface, ERR_INVALID_ENDPOINT, epnt);
	}
	else
		SCCI_SendErrorFrameEx(Interface, node, ERR_NOT_SUPPORTED, 0);
}
// ----------------------------------------

static void SCCI_HandleWriteBlock16(pSCCI_Interface Interface)
{
	Int16U node = Interface->MessageBuffer[0] & 0xFF;
	Int16U epnt = Interface->MessageBuffer[2] >> 8;
	Int16U length = Interface->MessageBuffer[2] & 0xFF;

	if(node == DEVICE_SCCI_ADDRESS)
	{
		if(length <= SCCI_BLOCK_MAX_VAL_16_W)
		{
			if((epnt < xCCI_MAX_WRITE_ENDPOINTS) && Interface->ProtectionAndEndpoints.WriteEndpoints16[epnt])
			{
				if(Interface->ProtectionAndEndpoints.WriteEndpoints16[epnt](epnt, &Interface->MessageBuffer[3], FALSE,
						length, Interface->ArgForEPCallback))
				{
					Interface->MessageBuffer[2] &= 0xFF00;
					SCCI_SendResponseFrame(Interface, 4);
				}
				else
					SCCI_SendErrorFrame(Interface, ERR_TOO_LONG, epnt);
			}
			else
				SCCI_SendErrorFrame(Interface, ERR_INVALID_ENDPOINT, epnt);
		}
		else
			SCCI_SendErrorFrame(Interface, ERR_ILLEGAL_SIZE, length);
	}
	else
	{
		
		Int16U err = BCCIM_WriteBlock16(&DEVICE_CAN_Interface, node, epnt, &Interface->MessageBuffer[3], length);

		if(err == ERR_NO_ERROR)
			SCCI_AnswerWriteBlock16(Interface, node, epnt);
		else
			SCCI_AnswerError(Interface, node, err, BCCIM_GetSavedErrorDetails());

	}
}
// ----------------------------------------

static void SCCI_HandleReadBlockFast16(pSCCI_Interface Interface, Boolean Repeat)
{
	pInt16U src;
	Int16U length;
	Int16U node = Interface->MessageBuffer[0] & 0xFF;
	Int16U epnt = Interface->MessageBuffer[2] >> 8;
	Int16U epnt_index;

	if(node == DEVICE_SCCI_ADDRESS)
	{
		if(xCCI_EndpointIndex(&Interface->ProtectionAndEndpoints, epnt, &epnt_index))
		{
			xCCI_FUNC_CallbackReadEndpoint16 Callback =
					(xCCI_FUNC_CallbackReadEndpoint16)Interface->ProtectionAndEndpoints.ReadEndpoints16[epnt_index].Callback;
			length = Callback(epnt_index + 1, &src, TRUE, Repeat, Interface->ArgForEPCallback, 0);

			Interface->MessageBuffer[2] = (epnt << 8) | (SCCI_USE_CRC_IN_STREAM ? 1 : 0);

			if(!length || (length > xCCI_BLOCK_STM_MAX_VAL))
				length = 0;

			Interface->MessageBuffer[3] = length;

			if(SCCI_USE_CRC_IN_STREAM)
				Interface->MessageBuffer[4] = CRC16_ComputeCRC(src, length);

			SCCI_SendResponseFrame(Interface, 6);

			Interface->IOConfig->IO_SendArray16(src, length);
			Interface->IOConfig->IO_SendArray16(ZeroBuffer, (8 - length % 8) % 8);
		}
		else
			SCCI_SendErrorFrame(Interface, ERR_INVALID_ENDPOINT, epnt);
	}
	else
	{
		Int16U err = BCCIM_ReadBlock16(&DEVICE_CAN_Interface, node, epnt);

		if(err == ERR_NO_ERROR)
			SCCI_AnswerReadBlock16Fast(Interface, node, epnt, BCCIM_ReadBlock16Buffer, BCCIM_ReadBlockBufferCounter);
		else
			SCCI_AnswerError(Interface, node, err, BCCIM_GetSavedErrorDetails());
	}
}
// ----------------------------------------

void SCCI_HandleReadBlockFastFloat(pSCCI_Interface Interface)
{
	Int16U node = Interface->MessageBuffer[0] & 0xFF;
	Int16U epnt = Interface->MessageBuffer[2] >> 8;

	if(node != DEVICE_SCCI_ADDRESS)
	{
		Int16U err = BCCIM_ReadBlockFloat(&DEVICE_CAN_Interface, node, epnt);

		if(err == ERR_NO_ERROR)
			SCCI_AnswerReadBlockFloatFast(Interface, node, epnt, BCCIM_ReadBlockFloatBuffer, BCCIM_ReadBlockBufferCounter);
		else
			SCCI_AnswerError(Interface, node, err, BCCIM_GetSavedErrorDetails());
	}
}
// ----------------------------------------

static void SCCI_HandleCall(pSCCI_Interface Interface)
{
	Int16U node = Interface->MessageBuffer[0] & 0xFF;
	Int16U action = Interface->MessageBuffer[2];

	if(node == DEVICE_SCCI_ADDRESS)
	{
		Int16U userError = 0;

		if(!Interface->ServiceConfig->UserActionCallback(action, &userError))
		{
			SCCI_SendErrorFrame(Interface, ERR_INVALID_ACTION, action);
		}
		else if(userError != 0)
		{
			SCCI_SendErrorFrame(Interface, ERR_USER, userError);
		}
		else
			SCCI_SendResponseFrame(Interface, 4);
	}
	else
	{
#ifdef MME_PROTECTED_CALL
		if(node == SM_TOU_NODE_ID && SM_IsTOUSwitchAction(action) && SM_IsCUHV2Connected())
		{
			SCCI_SendErrorFrameEx(Interface, node, ERR_BLOCKED, action);
			return;
		}

		if(node == SM_CUHV2_NODE_ID && SM_IsCUHV2SwitchAction(action) && SM_IsTOUConnected())
		{
			SCCI_SendErrorFrameEx(Interface, node, ERR_BLOCKED, action);
			return;
		}

		if(node == SM_CUHV2_NODE_ID)
			SM_ProcessCUHV2Command(action);

		if(node == SM_TOU_NODE_ID)
			SM_ProcessTOUCommand(action);
#endif
		Int16U err = BCCIM_Call(&DEVICE_CAN_Interface, node, action);

		if(err == ERR_NO_ERROR)
			SCCI_AnswerCall(Interface, node, action);
		else
			SCCI_AnswerError(Interface, node, err, BCCIM_GetSavedErrorDetails());
	}
}
// ----------------------------------------

static void SCCI_SendResponseFrame(pSCCI_Interface Interface, Int16U FrameSize)
{
	Interface->State = SCCI_STATE_WAIT_STARTBYTE;

	Interface->MessageBuffer[0] = (START_BYTE << 8) | DEVICE_SCCI_ADDRESS;
	Interface->MessageBuffer[1] |= RESPONSE_MASK << 8;
	Interface->MessageBuffer[FrameSize - 1] = CRC16_ComputeCRC(Interface->MessageBuffer, FrameSize - 1);

	Interface->IOConfig->IO_SendArray16(Interface->MessageBuffer, FrameSize);
}
// ----------------------------------------

static void SCCI_SendResponseFrameEx(pSCCI_Interface Interface, Int16U Node, Int16U Function, Int16U SubFunction,
		Int16U FrameSize)
{
	Interface->State = SCCI_STATE_WAIT_STARTBYTE;

	Interface->MessageBuffer[0] = (START_BYTE << 8) | Node;
	Interface->MessageBuffer[1] = (RESPONSE_MASK | (Function << 3) | SubFunction) << 8;
	Interface->MessageBuffer[FrameSize - 1] = CRC16_ComputeCRC(Interface->MessageBuffer, FrameSize - 1);

	Interface->IOConfig->IO_SendArray16(Interface->MessageBuffer, FrameSize);
}
// ----------------------------------------

static void SCCI_SendErrorFrame(pSCCI_Interface Interface, Int16U ErrorCode, Int16U Details)
{
	SCCI_SendErrorFrameEx(Interface, DEVICE_SCCI_ADDRESS, ErrorCode, Details);
}
// ----------------------------------------

static void SCCI_SendErrorFrameEx(pSCCI_Interface Interface, Int16U Node, Int16U ErrorCode, Int16U Details)
{
	Interface->State = SCCI_STATE_WAIT_STARTBYTE;

	Interface->MessageBuffer[0] = (START_BYTE << 8) | Node;
	Interface->MessageBuffer[1] = (RESPONSE_MASK | (FUNCTION_ERROR << 3)) << 8;
	Interface->MessageBuffer[2] = ErrorCode;
	Interface->MessageBuffer[3] = Details;
	Interface->MessageBuffer[4] = CRC16_ComputeCRC(Interface->MessageBuffer, 4);

	Interface->IOConfig->IO_SendArray16(Interface->MessageBuffer, 5);
}
// ----------------------------------------
