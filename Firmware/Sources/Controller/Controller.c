// -----------------------------------------
// Controller logic
// ----------------------------------------

// Header
#include "Controller.h"
//
// Includes
#include "SysConfig.h"
//
#include "ZbBoard.h"
#include "DeviceObjectDictionary.h"
#include "DataTable.h"
#include "DeviceProfile.h"
#include "StateMachine.h"

// Variables
//
volatile Int64U CONTROL_TimeCounter = 0;
volatile DeviceState CONTROL_State = DS_None;
static volatile Boolean CycleActive = FALSE;
//
Int16U CONTROL_Values_1[VALUES_x_SIZE] = {0};
volatile Int16U CONTROL_Values_1_Counter = 0;
//
Int16U CONTROL_CAN_Nodes[MAX_NODE_COUNT] = {0};
volatile Int16U CONTROL_CAN_Nodes_Counter = 0;
// Boot-loader flag
#pragma DATA_SECTION(CONTROL_BootLoaderRequest, "bl_flag");
volatile Int16U CONTROL_BootLoaderRequest = 0;

// Forward functions
//
static void CONTROL_FillWPPartDefault();
static Boolean CONTROL_DispatchAction(Int16U ActionID, pInt16U UserError);

// Functions
//
void CONTROL_Init()
{
	// Variables for endpoint configuration
	Int16U EPIndexes[EP_COUNT] = {EP1_CAN_Nodes, EP16_Data_1};
	Int16U EPSized[EP_COUNT] = {MAX_NODE_COUNT, VALUES_x_SIZE};
	pInt16U EPCounters[EP_COUNT] = {(pInt16U)&CONTROL_CAN_Nodes_Counter, (pInt16U)&CONTROL_Values_1_Counter};
	pInt16U EPDatas[EP_COUNT] = {CONTROL_Values_1, CONTROL_CAN_Nodes};
	// Data-table EPROM service configuration
	EPROMServiceConfig EPROMService = { &ZbMemory_WriteValuesEPROM, &ZbMemory_ReadValuesEPROM };
	
	// Init data table
	DT_Init(EPROMService, FALSE);
	DT_SaveFirmwareInfo(DEVICE_CAN_ADDRESS, 0);
	// Fill state variables with default values
	CONTROL_FillWPPartDefault();
	
	// Device profile initialization
	DEVPROFILE_Init(&CONTROL_DispatchAction, &CycleActive);
	DEVPROFILE_InitEPService(EPIndexes, EPSized, EPCounters, EPDatas);
	// Reset control values
	DEVPROFILE_ResetControlSection();
	DataTable[REG_MME_CODE] = DataTable[REG_MME_CODE_CONFIG];
	
	if(ZwSystem_GetDogAlarmFlag())
		ZwSystem_ClearDogAlarmFlag();
}
// ----------------------------------------

void CONTROL_Idle()
{
	DEVPROFILE_ProcessRequests();
	DEVPROFILE_UpdateCANDiagStatus();
}
// ----------------------------------------

#ifdef BOOT_FROM_FLASH
#pragma CODE_SECTION(CONTROL_NotifyCANFault, "ramfuncs");
#endif
void CONTROL_NotifyCANFault(ZwCAN_SysFlags Flag)
{
	DEVPROFILE_NotifyCANFault(Flag);
}
// ----------------------------------------

static void CONTROL_FillWPPartDefault()
{
	Int16U i;
	
	// Set states
	DataTable[REG_DEV_STATE] = DS_None;
	DataTable[REG_FAULT_REASON] = FAULT_NONE;
	DataTable[REG_WARNING] = WARNING_NONE;
	
	// Set results to zero
	for(i = REG_SENSOR_1; i <= REG_SENSOR_4; ++i)
		DataTable[i] = 0;
}
// ----------------------------------------

static Boolean CONTROL_DispatchAction(Int16U ActionID, pInt16U UserError)
{
	switch(ActionID)
	{
		case ACT_RESET_STATE_MACHINE:
			SM_Reset();
			break;

		case ACT_BOOT_LOADER_REQUEST:
			CONTROL_BootLoaderRequest = BOOT_LOADER_REQUEST;
			break;

		case ACT_BROADCAST_PING:
			BCCIM_SendBroadcastPing(&DEVICE_CAN_Interface, CONTROL_CAN_Nodes, (pInt16U)CONTROL_CAN_Nodes_Counter);
			break;

		default:
			return FALSE;
	}

	return TRUE;
}
// ----------------------------------------
