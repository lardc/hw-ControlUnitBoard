﻿// -----------------------------------------
// Device profile
// ----------------------------------------

// Header
#include "DeviceProfile.h"
//
// Includes
#include "SysConfig.h"
#include "Global.h"
#include "DeviceObjectDictionary.h"
#include "ZwDSP.h"
#include "ZbBoard.h"
#include "DataTable.h"
#include "Controller.h"
#include "Constraints.h"

// Types
//
typedef struct __EPState
{
	Int16U Size;
	Int16U ReadCounter;
	Int16U LastReadCounter;
	pInt16U pDataCounter;
	pInt16U Data;
} EPState, *pEPState;
//
typedef struct __EPStates
{
	EPState EPs[EP_COUNT];
} EPStates, *pEPStates;

// Variables
//
SCCI_Interface DEVICE_RS232_Interface;
BCCIM_Interface DEVICE_CAN_Interface;
//
static SCCI_IOConfig RS232_IOConfig;
static BCCI_IOConfig CAN_IOConfig;
static xCCI_ServiceConfig X_ServiceConfig;
static xCCI_FUNC_CallbackAction ControllerDispatchFunction;
static EPStates RS232_EPState;
static Boolean UnlockedForNVWrite = FALSE;
//
static volatile Boolean *MaskChangesFlag;

// Forward functions
//
static void DEVPROFILE_FillNVPartDefault();
static void DEVPROFILE_FillWRPartDefault();
static Boolean DEVPROFILE_Validate32(Int16U Address, Int32U Data);
static Boolean DEVPROFILE_Validate16(Int16U Address, Int16U Data);
static Boolean DEVPROFILE_DispatchAction(Int16U ActionID, pInt16U UserError);
static Int16U DEVPROFILE_CallbackReadX(Int16U Endpoint, pInt16U *Buffer, Boolean Streamed,
		Boolean RepeatLastTransmission, void *EPStateAddress, Int16U MaxNonStreamSize);

// Functions
//
void DEVPROFILE_Init(xCCI_FUNC_CallbackAction SpecializedDispatch, volatile Boolean *MaskChanges)
{
	// Save values
	ControllerDispatchFunction = SpecializedDispatch;
	MaskChangesFlag = MaskChanges;
	
	// Init interface
	RS232_IOConfig.IO_SendArray16 = &ZwSCIa_SendArray16;
	RS232_IOConfig.IO_ReceiveArray16 = &ZwSCIa_ReceiveArray16;
	RS232_IOConfig.IO_GetBytesToReceive = &ZwSCIa_GetBytesToReceive;
	RS232_IOConfig.IO_ReceiveByte = &ZwSCIa_ReceiveChar;
	
	CAN_IOConfig.IO_SendMessage = &ZwCANa_SendMessage;
	CAN_IOConfig.IO_SendMessageEx = &ZwCANa_SendMessageEx;
	CAN_IOConfig.IO_GetMessage = &ZwCANa_GetMessage;
	CAN_IOConfig.IO_IsMessageReceived = &ZwCANa_IsMessageReceived;
	CAN_IOConfig.IO_ConfigMailbox = &ZwCANa_ConfigMailbox;
	
	// Init service
	X_ServiceConfig.Read32Service = &DEVPROFILE_ReadValue32;
	X_ServiceConfig.Write32Service = &DEVPROFILE_WriteValue32;
	X_ServiceConfig.UserActionCallback = &DEVPROFILE_DispatchAction;
	X_ServiceConfig.ValidateCallback16 = &DEVPROFILE_Validate16;
	X_ServiceConfig.ValidateCallback32 = &DEVPROFILE_Validate32;
	
	// Init interface driver
	SCCI_Init(&DEVICE_RS232_Interface, &RS232_IOConfig, &X_ServiceConfig, (pInt16U)DataTable,
	DATA_TABLE_SIZE, SCCI_TIMEOUT_TICKS, &RS232_EPState);
	BCCIM_Init(&DEVICE_CAN_Interface, &CAN_IOConfig, SCCI_TIMEOUT_TICKS, &CONTROL_TimeCounter);
	
	// Set write protection
	SCCI_AddProtectedArea(&DEVICE_RS232_Interface, DATA_TABLE_WP_START, DATA_TABLE_SIZE - 1);
}
// ----------------------------------------

void DEVPROFILE_InitEPService(pInt16U Indexes, pInt16U Sizes, pInt16U *Counters, pInt16U *Datas)
{
	Int16U i;
	
	for(i = 0; i < EP_COUNT; ++i)
	{
		RS232_EPState.EPs[i].Size = Sizes[i];
		RS232_EPState.EPs[i].pDataCounter = Counters[i];
		RS232_EPState.EPs[i].Data = Datas[i];
		RS232_EPState.EPs[i].ReadCounter = RS232_EPState.EPs[i].LastReadCounter = 0;
		
		SCCI_RegisterReadEndpoint16(&DEVICE_RS232_Interface, Indexes[i], &DEVPROFILE_CallbackReadX);
	}
}
// ----------------------------------------
void DEVPROFILE_ProcessRequests()
{
	// Handle interface requests
	SCCI_Process(&DEVICE_RS232_Interface, CONTROL_TimeCounter, *MaskChangesFlag);
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
#pragma CODE_SECTION(DEVPROFILE_ResetEPReadState, "ramfuncs");
#endif
void DEVPROFILE_ResetEPReadState()
{
	Int16U i;
	
	for(i = 0; i < EP_COUNT; ++i)
	{
		RS232_EPState.EPs[i].ReadCounter = 0;
		RS232_EPState.EPs[i].LastReadCounter = 0;
	}
}
// ----------------------------------------

void DEVPROFILE_ResetControlSection()
{
	DT_ResetWRPart(&DEVPROFILE_FillWRPartDefault);
}
// ----------------------------------------

void DEVPROFILE_ResetScopes(Int16U ResetPosition)
{
	Int16U i;
	
	for(i = 0; i < EP_COUNT; ++i)
	{
		*(RS232_EPState.EPs[i].pDataCounter) = ResetPosition;
		MemZero16(RS232_EPState.EPs[i].Data, RS232_EPState.EPs[i].Size);
	}
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
#pragma CODE_SECTION(DEVPROFILE_NotifyCANFault, "ramfuncs");
#endif
void DEVPROFILE_NotifyCANFault(ZwCAN_SysFlags Flag)
{
	if(Flag & BOIM)
		DataTable[REG_CAN_BUSOFF_COUNTER]++;
}
// ----------------------------------------

void DEVPROFILE_UpdateCANDiagStatus()
{
	CANDiagnosticInfo info = ZwCANa_GetDiagnosticInfo();
	
	DEVPROFILE_WriteValue32((pInt16U)DataTable, REG_CAN_STATUS_REG, info.Status);
	DataTable[REG_CAN_DIAG_TEC] = info.TEC;
	DataTable[REG_CAN_DIAG_REC] = info.REC;
}
// ----------------------------------------

Int32U DEVPROFILE_ReadValue32(pInt16U pTable, Int16U Index)
{
	return pTable[Index] | (((Int32U)(pTable[Index + 1])) << 16);
}
// ----------------------------------------

void DEVPROFILE_WriteValue32(pInt16U pTable, Int16U Index, Int32U Data)
{
	pTable[Index] = Data & 0x0000FFFF;
	pTable[Index + 1] = Data >> 16;
}
// ----------------------------------------

static void DEVPROFILE_FillWRPartDefault()
{
	Int16U i;
	
	// Write default values to data table
	for(i = 0; i < (DATA_TABLE_WP_START - DATA_TABLE_WR_START); ++i)
		DataTable[DATA_TABLE_WR_START + i] = VConstraint[i].Default;
}
// ----------------------------------------

static Boolean DEVPROFILE_Validate16(Int16U Address, Int16U Data)
{
	if(ENABLE_LOCKING && !UnlockedForNVWrite && (Address < DATA_TABLE_WR_START))
		return FALSE;
	
	if(Address < DATA_TABLE_WR_START)
	{
		if(Data < NVConstraint[Address - DATA_TABLE_NV_START].Min
				|| Data > NVConstraint[Address - DATA_TABLE_NV_START].Max)
			return FALSE;
	}
	else if(Address < DATA_TABLE_WP_START)
	{
		if(Data < VConstraint[Address - DATA_TABLE_WR_START].Min
				|| Data > VConstraint[Address - DATA_TABLE_WR_START].Max)
			return FALSE;
	}
	
	return TRUE;
}
// ----------------------------------------

static Boolean DEVPROFILE_Validate32(Int16U Address, Int32U Data)
{
	if(ENABLE_LOCKING && !UnlockedForNVWrite && (Address < DATA_TABLE_WR_START))
		return FALSE;
	
	return FALSE;
}
// ----------------------------------------

static void DEVPROFILE_FillNVPartDefault()
{
	Int16U i;

	for(i = 0; i < DATA_TABLE_NV_SIZE; ++i)
		DataTable[DATA_TABLE_NV_START + i] = NVConstraint[i].Default;
}
// ----------------------------------------

static Boolean DEVPROFILE_DispatchAction(Int16U ActionID, pInt16U UserError)
{
	switch(ActionID)
	{
		case ACT_SAVE_TO_ROM:
			if(ENABLE_LOCKING && !UnlockedForNVWrite)
				*UserError = ERR_WRONG_PWD;
			else
				DT_SaveNVPartToEPROM();
			break;

		case ACT_RESTORE_FROM_ROM:
			if(ENABLE_LOCKING && !UnlockedForNVWrite)
				*UserError = ERR_WRONG_PWD;
			else
				DT_RestoreNVPartFromEPROM();
			break;

		case ACT_RESET_TO_DEFAULT:
			if(ENABLE_LOCKING && !UnlockedForNVWrite)
				*UserError = ERR_WRONG_PWD;
			else
				DT_ResetNVPart(&DEVPROFILE_FillNVPartDefault);
			break;

		default:
			return (ControllerDispatchFunction) ? ControllerDispatchFunction(ActionID, UserError) : FALSE;
	}

	return TRUE;
}
// ----------------------------------------

static Int16U DEVPROFILE_CallbackReadX(Int16U Endpoint, pInt16U *Buffer, Boolean Streamed,
		Boolean RepeatLastTransmission, void *EPStateAddress, Int16U MaxNonStreamSize)
{
	Int16U pLen;
	pEPState epState;
	pEPStates epStates = (pEPStates)EPStateAddress;
	
	// Validate pointer
	if(!epStates)
		return 0;
	
	// Get endpoint
	epState = &epStates->EPs[Endpoint - 1];
	
	// Handle transmission repeat
	if(RepeatLastTransmission)
		epState->ReadCounter = epState->LastReadCounter;
	
	// Write possible content reference
	*Buffer = epState->Data + epState->ReadCounter;
	
	// Calculate content length
	if(*(epState->pDataCounter) < epState->ReadCounter)
		pLen = 0;
	else
		pLen = *(epState->pDataCounter) - epState->ReadCounter;
	
	if(!Streamed)
		pLen = (pLen > MaxNonStreamSize) ? MaxNonStreamSize : pLen;
	
	// Update content state
	epState->LastReadCounter = epState->ReadCounter;
	epState->ReadCounter += pLen;
	
	return pLen;
}
// ----------------------------------------

