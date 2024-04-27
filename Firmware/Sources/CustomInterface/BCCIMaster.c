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
#define CAN_ID_W_32					14
#define CAN_ID_W_F					16
#define CAN_ID_R_16					20
#define CAN_ID_R_32					24
#define CAN_ID_R_F					26
#define CAN_ID_WB_16				30
#define CAN_ID_RB_16				40
#define CAN_ID_RB_F					42
#define CAN_ID_CALL					50
#define CAN_ID_ERR					61
//
#define MBOX_W_16					1
#define MBOX_W_16_A					2
#define MBOX_W_F					3
#define MBOX_W_F_A					4
#define MBOX_W_32					5
#define MBOX_W_32_A					6
#define MBOX_R_16					7
#define MBOX_R_16_A					8
#define MBOX_R_F					9
#define MBOX_R_F_A					10
#define MBOX_R_32					11
#define MBOX_R_32_A					12
#define MBOX_C						13
#define MBOX_C_A					14
#define MBOX_ERR_A					15
#define MBOX_RB_16					16
#define MBOX_RB_16_A				17
#define MBOX_WB_16					19
#define MBOX_WB_16_A				20
#define MBOX_RB_F					21
#define MBOX_RB_F_A					22
//
#define READ_BLOCK_FLOAT_BUFFER_SIZE	4000
#define READ_BLOCK_16_BUFFER_SIZE		(READ_BLOCK_FLOAT_BUFFER_SIZE * 2)

// Forward functions
//
static Boolean BCCIM_HandleReadBlock16(pBCCIM_Interface Interface);
void BCCIM_ReadBlock16Subfunction(pBCCIM_Interface Interface, Int16U Node, Int16U Endpoint, Boolean Start);
void BCCIM_ReadBlockFloatSubfunction(pBCCIM_Interface Interface, Int16U Node, Int16U Endpoint, Boolean Start);
Boolean BCCIM_HandleReadBlockFloat(pBCCIM_Interface Interface);
Int16U BCCIM_WaitResponse(pBCCIM_Interface Interface, Int16U Mailbox);
//
static void BCCIM_SendFrame(pBCCIM_Interface Interface, Int16U Mailbox, pCANMessage Message, Int32U Node,
		Int16U Command);
//
#pragma DATA_SECTION(BCCIM_ReadBlock16Buffer, "data_mem");
Int16U  BCCIM_ReadBlock16Buffer[READ_BLOCK_16_BUFFER_SIZE + 4];
pInt32U BCCIM_ReadBlockFloatBuffer = (pInt32U)BCCIM_ReadBlock16Buffer;
Int16U  BCCIM_ReadBlockBufferCounter = 0;
//
static Int16U ReadBlockSavedEndpoint, ReadBlockSavedNode;
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

	Interface->IOConfig->IO_ConfigMailbox(MBOX_R_F, CAN_ID_R_F, FALSE, 2, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, ZW_CAN_STRONG_MATCH);
	Interface->IOConfig->IO_ConfigMailbox(MBOX_R_F_A, CAN_ID_R_F + 1, TRUE, 6, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, CAN_ACCEPTANCE_MASK);

	Interface->IOConfig->IO_ConfigMailbox(MBOX_R_32, CAN_ID_R_32, FALSE, 2, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, ZW_CAN_STRONG_MATCH);
	Interface->IOConfig->IO_ConfigMailbox(MBOX_R_32_A, CAN_ID_R_32 + 1, TRUE, 6, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, CAN_ACCEPTANCE_MASK);

	Interface->IOConfig->IO_ConfigMailbox(MBOX_W_16, CAN_ID_W_16, FALSE, 4, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, ZW_CAN_STRONG_MATCH);
	Interface->IOConfig->IO_ConfigMailbox(MBOX_W_16_A, CAN_ID_W_16 + 1, TRUE, 2, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, CAN_ACCEPTANCE_MASK);

	Interface->IOConfig->IO_ConfigMailbox(MBOX_W_F, CAN_ID_W_F, FALSE, 6, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, ZW_CAN_STRONG_MATCH);
	Interface->IOConfig->IO_ConfigMailbox(MBOX_W_F_A, CAN_ID_W_F + 1, TRUE, 4, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, CAN_ACCEPTANCE_MASK);

	Interface->IOConfig->IO_ConfigMailbox(MBOX_W_32, CAN_ID_W_32, FALSE, 6, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, ZW_CAN_STRONG_MATCH);
	Interface->IOConfig->IO_ConfigMailbox(MBOX_W_32_A, CAN_ID_W_32 + 1, TRUE, 4, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, CAN_ACCEPTANCE_MASK);

	Interface->IOConfig->IO_ConfigMailbox(MBOX_C, CAN_ID_CALL, FALSE, 2, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, ZW_CAN_STRONG_MATCH);
	Interface->IOConfig->IO_ConfigMailbox(MBOX_C_A, CAN_ID_CALL + 1, TRUE, 2, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, CAN_ACCEPTANCE_MASK);

	Interface->IOConfig->IO_ConfigMailbox(MBOX_RB_16, CAN_ID_RB_16, FALSE, 2, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, ZW_CAN_STRONG_MATCH);
	Interface->IOConfig->IO_ConfigMailbox(MBOX_RB_16_A, CAN_ID_RB_16 + 1, TRUE, 8, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, CAN_ACCEPTANCE_MASK);

	Interface->IOConfig->IO_ConfigMailbox(MBOX_WB_16, CAN_ID_WB_16, FALSE, 8, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, ZW_CAN_STRONG_MATCH);
	Interface->IOConfig->IO_ConfigMailbox(MBOX_WB_16_A, CAN_ID_WB_16 + 1, TRUE, 8, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, CAN_ACCEPTANCE_MASK);

	Interface->IOConfig->IO_ConfigMailbox(MBOX_RB_F, CAN_ID_RB_F, FALSE, 2, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, ZW_CAN_STRONG_MATCH);
	Interface->IOConfig->IO_ConfigMailbox(MBOX_RB_F_A, CAN_ID_RB_F + 1, TRUE, 8, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, CAN_ACCEPTANCE_MASK);

	Interface->IOConfig->IO_ConfigMailbox(MBOX_ERR_A, CAN_ID_ERR, TRUE, 4, ZW_CAN_MBProtected, ZW_CAN_NO_PRIORITY, CAN_ACCEPTANCE_MASK);
}
// ----------------------------------------

Int16U BCCIM_Read16(pBCCIM_Interface Interface, Int16U Node, Int16U Address, pInt16U Data)
{
	Int16U ret;
	CANMessage message;

	// Clear input mailboxes
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

Int16U BCCIM_Read32bitTemplate(pBCCIM_Interface Interface, Int16U Node, Int16U Address, pInt32U Data,
	Int16U Mailbox, Int16U MailboxAnswer, Int16U CANID)
{
	Int16U ret;
	CANMessage message;

	// Clear input mailboxes
	Interface->IOConfig->IO_GetMessage(MBOX_ERR_A, NULL);
	Interface->IOConfig->IO_GetMessage(MailboxAnswer, NULL);
	
	message.HIGH.WORD.WORD_0 = Address;
	BCCIM_SendFrame(Interface, Mailbox, &message, Node, CANID);
	
	ret = BCCIM_WaitResponse(Interface, MailboxAnswer);
	if(ret == ERR_NO_ERROR)
	{
		Interface->IOConfig->IO_GetMessage(MailboxAnswer, &message);
		if(Data)
			*Data = ((Int32U)message.HIGH.WORD.WORD_1) << 16 | message.LOW.WORD.WORD_2;
	}

	return ret;
}
// ----------------------------------------

Int16U BCCIM_Read32(pBCCIM_Interface Interface, Int16U Node, Int16U Address, pInt32U Data)
{
	return BCCIM_Read32bitTemplate(Interface, Node, Address, Data, MBOX_R_32, MBOX_R_32_A, CAN_ID_R_32);
}
// ----------------------------------------

Int16U BCCIM_ReadFloat(pBCCIM_Interface Interface, Int16U Node, Int16U Address, pInt32U Data)
{
	return BCCIM_Read32bitTemplate(Interface, Node, Address, Data, MBOX_R_F, MBOX_R_F_A, CAN_ID_R_F);
}
// ----------------------------------------

Int16U BCCIM_Write16(pBCCIM_Interface Interface, Int16U Node, Int16U Address, Int16U Data)
{
	CANMessage message;

	// Clear input mailboxes
	Interface->IOConfig->IO_GetMessage(MBOX_ERR_A, NULL);
	Interface->IOConfig->IO_GetMessage(MBOX_W_16_A, NULL);
	
	message.HIGH.WORD.WORD_0 = Address;
	message.HIGH.WORD.WORD_1 = Data;
	BCCIM_SendFrame(Interface, MBOX_W_16, &message, Node, CAN_ID_W_16);

	return BCCIM_WaitResponse(Interface, MBOX_W_16_A);
}
// ----------------------------------------

Int16U BCCIM_WriteBlock16(pBCCIM_Interface Interface, Int16U Node, Int16U Endpoint, pInt16U Data, Int16U DataLength)
{
	CANMessage message;

	// Clear input mailboxes
	Interface->IOConfig->IO_GetMessage(MBOX_ERR_A, NULL);
	Interface->IOConfig->IO_GetMessage(MBOX_WB_16_A, NULL);

	// Compose and send message
	message.HIGH.WORD.WORD_0 = (Endpoint << 8) | DataLength;
	switch(DataLength)
	{
		case 3:
			message.LOW.WORD.WORD_3 = *(Data + 2);
		case 2:
			message.LOW.WORD.WORD_2 = *(Data + 1);
		case 1:
			message.HIGH.WORD.WORD_1 = *Data;
			break;
	}
	BCCIM_SendFrame(Interface, MBOX_WB_16, &message, Node, CAN_ID_WB_16);

	// Get response
	return BCCIM_WaitResponse(Interface, MBOX_WB_16_A);
}
// ----------------------------------------

Int16U BCCIM_Write32bitTemplate(pBCCIM_Interface Interface, Int16U Node, Int16U Address, Int32U Data,
	Int16U Mailbox, Int16U MailboxAnswer, Int16U CANID)
{
	CANMessage message;

	// Clear input mailboxes
	Interface->IOConfig->IO_GetMessage(MBOX_ERR_A, NULL);
	Interface->IOConfig->IO_GetMessage(MailboxAnswer, NULL);
	
	message.HIGH.WORD.WORD_0 = Address;
	message.HIGH.WORD.WORD_1 = Data >> 16;
	message.LOW.WORD.WORD_2 = Data & 0xFFFF;
	BCCIM_SendFrame(Interface, Mailbox, &message, Node, CANID);

	return BCCIM_WaitResponse(Interface, MailboxAnswer);
}
// ----------------------------------------

Int16U BCCIM_Write32(pBCCIM_Interface Interface, Int16U Node, Int16U Address, Int32U Data)
{
	return BCCIM_Write32bitTemplate(Interface, Node, Address, Data, MBOX_W_32, MBOX_W_32_A, CAN_ID_W_32);
}
// ----------------------------------------

Int16U BCCIM_WriteFloat(pBCCIM_Interface Interface, Int16U Node, Int16U Address, Int32U Data)
{
	return BCCIM_Write32bitTemplate(Interface, Node, Address, Data, MBOX_W_F, MBOX_W_F_A, CAN_ID_W_F);
}
// ----------------------------------------

Int16U BCCIM_Call(pBCCIM_Interface Interface, Int16U Node, Int16U Action)
{
	CANMessage message;

	// Clear input mailboxes
	Interface->IOConfig->IO_GetMessage(MBOX_ERR_A, NULL);
	Interface->IOConfig->IO_GetMessage(MBOX_C_A, NULL);
	
	message.HIGH.WORD.WORD_0 = Action;
	BCCIM_SendFrame(Interface, MBOX_C, &message, Node, CAN_ID_CALL);

	return BCCIM_WaitResponse(Interface, MBOX_C_A);
}
// ----------------------------------------

Int16U BCCIM_ReadBlock16(pBCCIM_Interface Interface, Int16U Node, Int16U Endpoint)
{
	Int16U ret;
	Int64U timeout;
	BCCIM_ReadBlock16Subfunction(Interface, Node, Endpoint, TRUE);

	timeout = Interface->TimeoutValueTicks + *(Interface->pTimerCounter);
	while(*(Interface->pTimerCounter) < timeout)
	{
		// Get response
		if ((ret = BCCIM_WaitResponse(Interface, MBOX_RB_16_A)) == ERR_NO_ERROR)
		{
			if (BCCIM_HandleReadBlock16(Interface))
				return ERR_NO_ERROR;
		}
		else
			return ret;
	}

	return ERR_TIMEOUT;
}
// ----------------------------------------

void BCCIM_ReadBlock16Subfunction(pBCCIM_Interface Interface, Int16U Node, Int16U Endpoint, Boolean Start)
{
	CANMessage message;

	if(Start)
	{
		// Clear input mailboxes
		Interface->IOConfig->IO_GetMessage(MBOX_ERR_A, NULL);
		Interface->IOConfig->IO_GetMessage(MBOX_RB_16_A, NULL);

		ReadBlockSavedEndpoint = Endpoint;
		ReadBlockSavedNode = Node;
		BCCIM_ReadBlockBufferCounter = 0;
	}

	message.HIGH.WORD.WORD_0 = ReadBlockSavedEndpoint;
	BCCIM_SendFrame(Interface, MBOX_RB_16, &message, ReadBlockSavedNode, CAN_ID_RB_16);
}
// ----------------------------------------

Boolean BCCIM_HandleReadBlock16(pBCCIM_Interface Interface)
{
	CANMessage CANInput;
	Interface->IOConfig->IO_GetMessage(MBOX_RB_16_A, &CANInput);

	if(BCCIM_ReadBlockBufferCounter >= READ_BLOCK_16_BUFFER_SIZE)
		return TRUE;

	switch(CANInput.DLC)
	{
		case 8:
			BCCIM_ReadBlock16Buffer[BCCIM_ReadBlockBufferCounter + 3] = CANInput.LOW.WORD.WORD_3;
		case 6:
			BCCIM_ReadBlock16Buffer[BCCIM_ReadBlockBufferCounter + 2] = CANInput.LOW.WORD.WORD_2;
		case 4:
			BCCIM_ReadBlock16Buffer[BCCIM_ReadBlockBufferCounter + 1] = CANInput.HIGH.WORD.WORD_1;
		case 2:
			BCCIM_ReadBlock16Buffer[BCCIM_ReadBlockBufferCounter] = CANInput.HIGH.WORD.WORD_0;
			BCCIM_ReadBlockBufferCounter += CANInput.DLC / 2;
			BCCIM_ReadBlock16Subfunction(Interface, 0, 0, FALSE);
			return FALSE;
		default:
			return TRUE;
	}
}
// ----------------------------------------

Int16U BCCIM_ReadBlockFloat(pBCCIM_Interface Interface, Int16U Node, Int16U Endpoint)
{
	Int16U ret;
	Int64U timeout;
	BCCIM_ReadBlockFloatSubfunction(Interface, Node, Endpoint, TRUE);

	timeout = Interface->TimeoutValueTicks + *(Interface->pTimerCounter);
	while(*(Interface->pTimerCounter) < timeout)
	{
		// Get response
		if ((ret = BCCIM_WaitResponse(Interface, MBOX_RB_F_A)) == ERR_NO_ERROR)
		{
			if (BCCIM_HandleReadBlockFloat(Interface))
				return ERR_NO_ERROR;
		}
		else
			return ret;
	}

	return ERR_TIMEOUT;
}
// ----------------------------------------

void BCCIM_ReadBlockFloatSubfunction(pBCCIM_Interface Interface, Int16U Node, Int16U Endpoint, Boolean Start)
{
	CANMessage message;

	if(Start)
	{
		// Clear input mailboxes
		Interface->IOConfig->IO_GetMessage(MBOX_ERR_A, NULL);
		Interface->IOConfig->IO_GetMessage(MBOX_RB_F_A, NULL);

		ReadBlockSavedEndpoint = Endpoint;
		ReadBlockSavedNode = Node;
		BCCIM_ReadBlockBufferCounter = 0;
	}

	message.HIGH.WORD.WORD_0 = ReadBlockSavedEndpoint;
	BCCIM_SendFrame(Interface, MBOX_RB_F, &message, ReadBlockSavedNode, CAN_ID_RB_F);
}
// ----------------------------------------

Boolean BCCIM_HandleReadBlockFloat(pBCCIM_Interface Interface)
{
	CANMessage CANInput;
	Interface->IOConfig->IO_GetMessage(MBOX_RB_F_A, &CANInput);

	if(BCCIM_ReadBlockBufferCounter >= READ_BLOCK_FLOAT_BUFFER_SIZE)
		return TRUE;

	Int16U length = CANInput.DLC / 4;
	switch(length)
	{
		case 2:
			BCCIM_ReadBlockFloatBuffer[BCCIM_ReadBlockBufferCounter + 1] = CANInput.LOW.DWORD_1;
		case 1:
			BCCIM_ReadBlockFloatBuffer[BCCIM_ReadBlockBufferCounter] = CANInput.HIGH.DWORD_0;

			BCCIM_ReadBlockBufferCounter += length;
			BCCIM_ReadBlockFloatSubfunction(Interface, 0, 0, FALSE);
			return FALSE;
		default:
			return TRUE;
	}
}
// ----------------------------------------

static void BCCIM_SendFrame(pBCCIM_Interface Interface, Int16U Mailbox, pCANMessage Message, Int32U Node,
		Int16U Command)
{
	Message->MsgID.all = (Node << 10) | Command;
	
	Interface->IOConfig->IO_SendMessageEx(Mailbox, Message, TRUE, FALSE);
}
// ----------------------------------------

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
// ----------------------------------------

Int16U BCCIM_GetSavedErrorDetails()
{
	return SavedErrorDetails;
}
// ----------------------------------------
