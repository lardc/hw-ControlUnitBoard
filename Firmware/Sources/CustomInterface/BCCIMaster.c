// -----------------------------------------
// BCCI communication interface
// ----------------------------------------

// Header
#include "BCCIMaster.h"
//
// Includes
#include "CRC16.h"
#include "SysConfig.h"
#include "DeviceProfile.h"
#include "StateMachine.h"

// Macro
//
#define CAN_ID_W_16					10
#define CAN_ID_W_16_2				12
#define CAN_ID_W_32					14
#define CAN_ID_R_16					20
#define CAN_ID_R_16_2				22
#define CAN_ID_R_32					24
#define CAN_ID_WB_16				30
#define CAN_ID_RB_16				40
#define CAN_ID_CALL					50
#define CAN_ID_ERR					61
//
#define MBOX_W_16					1
#define MBOX_W_16_A					2
#define MBOX_W_16_2					3
#define MBOX_W_16_2_A				4
#define MBOX_W_32					5
#define MBOX_W_32_A					6
#define MBOX_R_16					7
#define MBOX_R_16_A					8
#define MBOX_R_16_2					9
#define MBOX_R_16_2_A				10
#define MBOX_R_32					11
#define MBOX_R_32_A					12
#define MBOX_C						13
#define MBOX_C_A					14
#define MBOX_ERR_A					15
#define MBOX_RB_16					16
#define MBOX_RB_16_A				17
#define MBOX_WB_16					19
#define MBOX_WB_16_A				20
//
#define READ_BLOCK_16_BUFFER_SIZE	8000

// Forward functions
//
static void BCCIM_HandleRead16(pBCCIM_Interface Interface);
static void BCCIM_HandleRead16Double(pBCCIM_Interface Interface);
static void BCCIM_HandleRead32(pBCCIM_Interface Interface);
static void BCCIM_HandleWrite16(pBCCIM_Interface Interface);
static void BCCIM_HandleWrite16Double(pBCCIM_Interface Interface);
static void BCCIM_HandleWriteBlock16(pBCCIM_Interface Interface);
static void BCCIM_HandleWrite32(pBCCIM_Interface Interface);
static void BCCIM_HandleCall(pBCCIM_Interface Interface);
static void BCCIM_HandleReadBlock16(pBCCIM_Interface Interface);
static void BCCIM_HandleError(pBCCIM_Interface Interface);
Int16U BCCIM_WaitResponse(pBCCIM_Interface Interface, Int16U Mailbox);
//
static void BCCIM_SendFrame(pBCCIM_Interface Interface, Int16U Mailbox, pCANMessage Message, Int32U Node,
		Int16U Command);
//
#pragma DATA_SECTION(ReadBlock16Buffer, "data_mem");
static Int16U ReadBlock16Buffer[READ_BLOCK_16_BUFFER_SIZE + 4];
static Int16U ReadBlock16BufferCounter = 0;
//
static Int16U SavedEndpointRB16;
static pSCCI_Interface ActiveSCCI;
static Int16U SavedErrorDetails = 0;

// Functions
//
void BCCIM_Init(pBCCIM_Interface Interface, pBCCI_IOConfig IOConfig, Int32U MessageTimeoutTicks, volatile Int64U *pTimer)
{
	// Save parameters
	Interface->IOConfig = IOConfig;
	Interface->TimeoutValueTicks = MessageTimeoutTicks;
	Interface->pTimerCounter = pTimer;

	// Setup messages
	Interface->IOConfig->IO_ConfigMailbox(MBOX_R_16, CAN_ID_R_16, FALSE, 2, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, ZW_CAN_STRONG_MATCH);
	Interface->IOConfig->IO_ConfigMailbox(MBOX_R_16_A, CAN_ID_R_16 + 1, TRUE, 4, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, CAN_ACCEPTANCE_MASK);

	Interface->IOConfig->IO_ConfigMailbox(MBOX_R_16_2, CAN_ID_R_16_2, FALSE, 4, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, ZW_CAN_STRONG_MATCH);
	Interface->IOConfig->IO_ConfigMailbox(MBOX_R_16_2_A, CAN_ID_R_16_2 + 1, TRUE, 8, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, CAN_ACCEPTANCE_MASK);

	Interface->IOConfig->IO_ConfigMailbox(MBOX_R_32, CAN_ID_R_32, FALSE, 2, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, ZW_CAN_STRONG_MATCH);
	Interface->IOConfig->IO_ConfigMailbox(MBOX_R_32_A, CAN_ID_R_32 + 1, TRUE, 6, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, CAN_ACCEPTANCE_MASK);

	Interface->IOConfig->IO_ConfigMailbox(MBOX_W_16, CAN_ID_W_16, FALSE, 4, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, ZW_CAN_STRONG_MATCH);
	Interface->IOConfig->IO_ConfigMailbox(MBOX_W_16_A, CAN_ID_W_16 + 1, TRUE, 2, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, CAN_ACCEPTANCE_MASK);

	Interface->IOConfig->IO_ConfigMailbox(MBOX_W_16_2, CAN_ID_W_16_2, FALSE, 8, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, ZW_CAN_STRONG_MATCH);
	Interface->IOConfig->IO_ConfigMailbox(MBOX_W_16_2_A, CAN_ID_W_16_2 + 1, TRUE, 4, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, CAN_ACCEPTANCE_MASK);

	Interface->IOConfig->IO_ConfigMailbox(MBOX_W_32, CAN_ID_W_32, FALSE, 6, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, ZW_CAN_STRONG_MATCH);
	Interface->IOConfig->IO_ConfigMailbox(MBOX_W_32_A, CAN_ID_W_32 + 1, TRUE, 4, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, CAN_ACCEPTANCE_MASK);

	Interface->IOConfig->IO_ConfigMailbox(MBOX_C, CAN_ID_CALL, FALSE, 2, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, ZW_CAN_STRONG_MATCH);
	Interface->IOConfig->IO_ConfigMailbox(MBOX_C_A, CAN_ID_CALL + 1, TRUE, 2, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, CAN_ACCEPTANCE_MASK);

	Interface->IOConfig->IO_ConfigMailbox(MBOX_RB_16, CAN_ID_RB_16, FALSE, 2, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, ZW_CAN_STRONG_MATCH);
	Interface->IOConfig->IO_ConfigMailbox(MBOX_RB_16_A, CAN_ID_RB_16 + 1, TRUE, 8, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, CAN_ACCEPTANCE_MASK);

	Interface->IOConfig->IO_ConfigMailbox(MBOX_WB_16, CAN_ID_WB_16, FALSE, 8, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, ZW_CAN_STRONG_MATCH);
	Interface->IOConfig->IO_ConfigMailbox(MBOX_WB_16_A, CAN_ID_WB_16 + 1, TRUE, 8, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, CAN_ACCEPTANCE_MASK);

	Interface->IOConfig->IO_ConfigMailbox(MBOX_ERR_A, CAN_ID_ERR, TRUE, 4, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, CAN_ACCEPTANCE_MASK);
}
// ----------------------------------------

void BCCIM_Process(pBCCIM_Interface Interface)
{
	if(Interface->IOConfig->IO_IsMessageReceived(MBOX_WB_16_A, NULL)) { BCCIM_HandleWriteBlock16(Interface); return; }
	if(Interface->IOConfig->IO_IsMessageReceived(MBOX_R_16_A, NULL)) { BCCIM_HandleRead16(Interface); return; }
	if(Interface->IOConfig->IO_IsMessageReceived(MBOX_R_16_2_A, NULL)) { BCCIM_HandleRead16Double(Interface); return; }
	if(Interface->IOConfig->IO_IsMessageReceived(MBOX_R_32_A, NULL)) { BCCIM_HandleRead32(Interface); return; }
	if(Interface->IOConfig->IO_IsMessageReceived(MBOX_W_16_A, NULL)) { BCCIM_HandleWrite16(Interface); return; }
	if(Interface->IOConfig->IO_IsMessageReceived(MBOX_W_16_2_A, NULL)) { BCCIM_HandleWrite16Double(Interface); return; }
	if(Interface->IOConfig->IO_IsMessageReceived(MBOX_W_32_A, NULL)) { BCCIM_HandleWrite32(Interface); return; }
	if(Interface->IOConfig->IO_IsMessageReceived(MBOX_C_A, NULL)) { BCCIM_HandleCall(Interface); return; }
	if(Interface->IOConfig->IO_IsMessageReceived(MBOX_RB_16_A, NULL)) { BCCIM_HandleReadBlock16(Interface); return; }
	if(Interface->IOConfig->IO_IsMessageReceived(MBOX_ERR_A, NULL)) { BCCIM_HandleError(Interface); return; }
}
// ----------------------------------------

Int16U BCCIM_Read16(pBCCIM_Interface Interface, Int16U Node, Int16U Address, pInt16U Data)
{
	Int16U ret;
	CANMessage message;
	
	Interface->IOConfig->IO_GetMessage(MBOX_ERR_A, NULL);
	Interface->IOConfig->IO_GetMessage(MBOX_R_16_A, NULL);
	
	message.HIGH.WORD.WORD_0 = Address;
	BCCIM_SendFrame(Interface, MBOX_R_16, &message, Node, CAN_ID_R_16);

	ret = BCCIM_WaitResponse(Interface, MBOX_R_16_A);
	if(ret == ERR_NO_ERROR)
	{
		Interface->IOConfig->IO_GetMessage(MBOX_R_16_A, &message);
		if(Data)
			*Data = message.HIGH.WORD.WORD_1;
	}

	return ret;
}
// ----------------------------------------

Int16U BCCIM_Read32(pBCCIM_Interface Interface, Int16U Node, Int16U Address, pInt32U Data)
{
	Int16U ret;
	CANMessage message;

	Interface->IOConfig->IO_GetMessage(MBOX_ERR_A, NULL);
	Interface->IOConfig->IO_GetMessage(MBOX_R_32_A, NULL);
	
	message.HIGH.WORD.WORD_0 = Address;
	BCCIM_SendFrame(Interface, MBOX_R_32, &message, Node, CAN_ID_R_32);
	
	ret = BCCIM_WaitResponse(Interface, MBOX_R_32_A);
	if(ret == ERR_NO_ERROR)
	{
		Interface->IOConfig->IO_GetMessage(MBOX_R_32_A, &message);
		if(Data)
			*Data = (Int32U)message.HIGH.WORD.WORD_1 << 16 | message.LOW.WORD.WORD_2;
	}

	return ret;

}
// ----------------------------------------

Int16U BCCIM_Write16(pBCCIM_Interface Interface, Int16U Node, Int16U Address, Int16U Data)
{
	CANMessage message;

	Interface->IOConfig->IO_GetMessage(MBOX_ERR_A, NULL);
	Interface->IOConfig->IO_GetMessage(MBOX_W_16_A, NULL);
	
	message.HIGH.WORD.WORD_0 = Address;
	message.HIGH.WORD.WORD_1 = Data;
	BCCIM_SendFrame(Interface, MBOX_W_16, &message, Node, CAN_ID_W_16);

	return BCCIM_WaitResponse(Interface, MBOX_W_16_A);
}
// ----------------------------------------

void BCCIM_WriteBlock16(pBCCIM_Interface Interface, Int16U Node, Int16U Endpoint, pInt16U Data, Int16U DataLength)
{
	CANMessage message;
	
	message.HIGH.WORD.WORD_0 = (Endpoint << 8) | DataLength;
	message.HIGH.WORD.WORD_1 = *Data;
	message.LOW.WORD.WORD_2 = *(Data + 1);
	message.LOW.WORD.WORD_3 = *(Data + 2);
	BCCIM_SendFrame(Interface, MBOX_WB_16, &message, Node, CAN_ID_WB_16);
}
// ----------------------------------------

Int16U BCCIM_Write32(pBCCIM_Interface Interface, Int16U Node, Int16U Address, Int32U Data)
{
	CANMessage message;

	Interface->IOConfig->IO_GetMessage(MBOX_ERR_A, NULL);
	Interface->IOConfig->IO_GetMessage(MBOX_W_32_A, NULL);
	
	message.HIGH.WORD.WORD_0 = Address;
	message.HIGH.WORD.WORD_1 = Data >> 16;
	message.LOW.WORD.WORD_2 = Data & 0xFFFF;
	BCCIM_SendFrame(Interface, MBOX_W_32, &message, Node, CAN_ID_W_32);

	return BCCIM_WaitResponse(Interface, MBOX_W_32_A);
}
// ----------------------------------------

void BCCIM_Call(pBCCIM_Interface Interface, Int16U Node, Int16U Action)
{
	CANMessage message;
	
	message.HIGH.WORD.WORD_0 = Action;
	BCCIM_SendFrame(Interface, MBOX_C, &message, Node, CAN_ID_CALL);
}
// ----------------------------------------

void BCCIM_ReadBlock16(pBCCIM_Interface Interface, Int16U Node, Int16U Endpoint, Boolean Start)
{
	CANMessage message;
	
	if(Start)
	{
		SavedEndpointRB16 = Endpoint;
		ReadBlock16BufferCounter = 0;
	}
	
	message.HIGH.WORD.WORD_0 = Endpoint;
	BCCIM_SendFrame(Interface, MBOX_RB_16, &message, Node, CAN_ID_RB_16);
}
// ----------------------------------------

static void BCCIM_HandleRead16(pBCCIM_Interface Interface)
{
	Int16U addr, data, node;
	CANMessage CANInput;
	
	Interface->IOConfig->IO_GetMessage(MBOX_R_16_A, &CANInput);
	addr = CANInput.HIGH.WORD.WORD_0;
	data = CANInput.HIGH.WORD.WORD_1;
	node = CANInput.MsgID.all >> 10;

#ifdef MME_PROTECTED_CALL
	if(node == SM_TOU_NODE_ID)
		SM_ProcessTOURegisterRead(addr, data);
#endif

	SCCI_AnswerRead16(ActiveSCCI, node, addr, data);
}
// ----------------------------------------

static void BCCIM_HandleRead16Double(pBCCIM_Interface Interface)
{
	Int16U addr1, data1, addr2, data2;
	CANMessage CANInput;
	
	Interface->IOConfig->IO_GetMessage(MBOX_R_16_2_A, &CANInput);
	addr1 = CANInput.HIGH.WORD.WORD_0;
	data1 = CANInput.HIGH.WORD.WORD_1;
	addr2 = CANInput.LOW.WORD.WORD_2;
	data2 = CANInput.LOW.WORD.WORD_3;
	
	SCCI_AnswerRead16Double(ActiveSCCI, CANInput.MsgID.all >> 10, addr1, data1, addr2, data2);
}
// ----------------------------------------

static void BCCIM_HandleRead32(pBCCIM_Interface Interface)
{
	Int16U addr;
	Int32U data;
	CANMessage CANInput;
	
	Interface->IOConfig->IO_GetMessage(MBOX_R_32_A, &CANInput);
	addr = CANInput.HIGH.WORD.WORD_0;
	data = ((Int32U)CANInput.HIGH.WORD.WORD_1 << 16) | CANInput.LOW.WORD.WORD_2;
	
	SCCI_AnswerRead32(ActiveSCCI, CANInput.MsgID.all >> 10, addr, data);
}
// ----------------------------------------

static void BCCIM_HandleWrite16(pBCCIM_Interface Interface)
{
	Int16U addr;
	CANMessage CANInput;
	
	Interface->IOConfig->IO_GetMessage(MBOX_W_16_A, &CANInput);
	addr = CANInput.HIGH.WORD.WORD_0;
	
	SCCI_AnswerWrite16(ActiveSCCI, CANInput.MsgID.all >> 10, addr);
}
// ----------------------------------------

static void BCCIM_HandleWrite16Double(pBCCIM_Interface Interface)
{
	Int16U addr1, addr2;
	CANMessage CANInput;
	
	Interface->IOConfig->IO_GetMessage(MBOX_W_16_2_A, &CANInput);
	addr1 = CANInput.HIGH.WORD.WORD_0;
	addr2 = CANInput.HIGH.WORD.WORD_1;
	
	SCCI_AnswerWrite16Double(ActiveSCCI, CANInput.MsgID.all >> 10, addr1, addr2);
}
// ----------------------------------------

static void BCCIM_HandleWriteBlock16(pBCCIM_Interface Interface)
{
	Int16U Data[4];
	CANMessage CANInput;
	
	Interface->IOConfig->IO_GetMessage(MBOX_WB_16_A, &CANInput);
	Data[0] = CANInput.HIGH.WORD.WORD_0;
	Data[1] = CANInput.HIGH.WORD.WORD_1;
	Data[2] = CANInput.LOW.WORD.WORD_2;
	Data[3] = CANInput.LOW.WORD.WORD_3;
	
	SCCI_AnswerWriteBlock16(ActiveSCCI, CANInput.MsgID.all >> 10, &Data[0]);
}
// ----------------------------------------

static void BCCIM_HandleWrite32(pBCCIM_Interface Interface)
{
	Int16U addr;
	CANMessage CANInput;
	
	Interface->IOConfig->IO_GetMessage(MBOX_W_32_A, &CANInput);
	addr = CANInput.HIGH.WORD.WORD_0;
	
	SCCI_AnswerWrite32(ActiveSCCI, CANInput.MsgID.all >> 10, addr);
}
// ----------------------------------------

static void BCCIM_HandleCall(pBCCIM_Interface Interface)
{
	Int16U action;
	CANMessage CANInput;
	
	Interface->IOConfig->IO_GetMessage(MBOX_C_A, &CANInput);
	action = CANInput.HIGH.WORD.WORD_0;
	
	SCCI_AnswerCall(ActiveSCCI, CANInput.MsgID.all >> 10, action);
}
// ----------------------------------------

static void BCCIM_HandleReadBlock16(pBCCIM_Interface Interface)
{
	CANMessage CANInput;
	
	Interface->IOConfig->IO_GetMessage(MBOX_RB_16_A, &CANInput);
	
	if(ReadBlock16BufferCounter >= READ_BLOCK_16_BUFFER_SIZE)
	{
		SCCI_AnswerError(ActiveSCCI, CANInput.MsgID.all >> 10, ERR_TOO_LONG, SavedEndpointRB16);
		return;
	}
	
	switch (CANInput.DLC)
	{
		case 8:
			ReadBlock16Buffer[ReadBlock16BufferCounter + 3] = CANInput.LOW.WORD.WORD_3;
		case 6:
			ReadBlock16Buffer[ReadBlock16BufferCounter + 2] = CANInput.LOW.WORD.WORD_2;
		case 4:
			ReadBlock16Buffer[ReadBlock16BufferCounter + 1] = CANInput.HIGH.WORD.WORD_1;
		case 2:
			ReadBlock16Buffer[ReadBlock16BufferCounter] = CANInput.HIGH.WORD.WORD_0;
			ReadBlock16BufferCounter += CANInput.DLC / 2;
			BCCIM_ReadBlock16(&DEVICE_CAN_Interface, CANInput.MsgID.all >> 10, SavedEndpointRB16, FALSE);
			break;
		default:
			SCCI_AnswerReadBlock16Fast(ActiveSCCI, CANInput.MsgID.all >> 10, SavedEndpointRB16, ReadBlock16Buffer,
					ReadBlock16BufferCounter);
			break;
	}
}
// ----------------------------------------

static void BCCIM_HandleError(pBCCIM_Interface Interface)
{
	Int16U code, details;
	CANMessage CANInput;
	
	Interface->IOConfig->IO_GetMessage(MBOX_ERR_A, &CANInput);
	code = CANInput.HIGH.WORD.WORD_0;
	details = CANInput.HIGH.WORD.WORD_1;
	
	SCCI_AnswerError(ActiveSCCI, CANInput.MsgID.all >> 10, code, details);
}
// ----------------------------------------

static void BCCIM_SendFrame(pBCCIM_Interface Interface, Int16U Mailbox, pCANMessage Message, Int32U Node,
		Int16U Command)
{
	Message->MsgID.all = (Node << 10) | Command;
	
	Interface->IOConfig->IO_SendMessageEx(Mailbox, Message, TRUE, FALSE);
}

Int16U BCCIM_WaitResponse(pBCCIM_Interface Interface, Int16U Mailbox)
{
	Int64U timeout;
	CANMessage message;

	// Wait for response
	timeout = Interface->TimeoutValueTicks + *(Interface->pTimerCounter);
	while(*(Interface->pTimerCounter) < timeout)
	{
		// In case of error
		if(Interface->IOConfig->IO_IsMessageReceived(MBOX_ERR_A, NULL))
		{
			Interface->IOConfig->IO_GetMessage(MBOX_ERR_A, &message);
			SavedErrorDetails = message.HIGH.WORD.WORD_1;
			return message.HIGH.WORD.WORD_0;
		}
		else if(Interface->IOConfig->IO_IsMessageReceived(Mailbox, NULL))
			return ERR_NO_ERROR;
	}
	return ERR_TIMEOUT;
}

Int16U BCCIM_GetSavedErrorDetails()
{
	return SavedErrorDetails;
}
// No more.
