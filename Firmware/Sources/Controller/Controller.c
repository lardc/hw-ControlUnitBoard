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
void CONTROL_InitCAN();
Int16U CONTROL_GetNodeID();

// Functions
//
void CONTROL_Init()
{
	// Variables for endpoint configuration
	Int16U EPIndexes[EP_COUNT] = {EP16_Data_1, EP16_CAN_Nodes};
	Int16U EPSized[EP_COUNT] = {VALUES_x_SIZE, MAX_NODE_COUNT};
	pInt16U EPCounters[EP_COUNT] = {(pInt16U)&CONTROL_Values_1_Counter, (pInt16U)&CONTROL_CAN_Nodes_Counter};
	pInt16U EPDatas[EP_COUNT] = {CONTROL_Values_1, CONTROL_CAN_Nodes};
	// Data-table EPROM service configuration
	EPROMServiceConfig EPROMService = { &ZbMemory_WriteValuesEPROM, &ZbMemory_ReadValuesEPROM };
	
	// Init data table
	DT_Init(EPROMService, FALSE);
	CONTROL_InitCAN();

	// Fill state variables with default values
	CONTROL_FillWPPartDefault();
	
	// Device profile initialization
	Int16U NodeID = CONTROL_GetNodeID();
	// Патч для работы с таблеточным ЗУ на ударном токе
	Int16U MasterNID = (NodeID == 1) ? 0 : NodeID;

	DEVPROFILE_Init(&CONTROL_DispatchAction, &CycleActive, NodeID, MasterNID);
	DEVPROFILE_InitEPService(EPIndexes, EPSized, EPCounters, EPDatas);
	DT_SaveFirmwareInfo(NodeID, MasterNID);

	// Reset control values
	DEVPROFILE_ResetControlSection();
	DataTable[REG_MME_CODE] = DataTable[REG_MME_CODE_CONFIG];
	DataTable[REG_WORKPLACE_FLAG] = 0;
	
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

Int16U CONTROL_GetNodeID()
{
	return (DataTable[REG_CAN_NID] == 0xFFFF || DataTable[REG_CAN_NID] == 0) ? DEVICE_CAN_ADDRESS : DataTable[REG_CAN_NID];
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

		case ACT_REINIT_CAN:
			{
				CONTROL_InitCAN();

				Int16U NodeID = CONTROL_GetNodeID();
				// Патч для работы с таблеточным ЗУ на ударном токе
				Int16U MasterNID = (NodeID == 1) ? 0 : NodeID;

				DEVPROFILE_BCCIx_InitWrapper(NodeID, MasterNID);
				DT_SaveFirmwareInfo(NodeID, MasterNID);
			}
			break;

		case ACT_BOOT_LOADER_REQUEST:
			CONTROL_BootLoaderRequest = BOOT_LOADER_REQUEST;
			break;

		case ACT_BROADCAST_PING:
			DEVPROFILE_ResetScopes(0);
			DEVPROFILE_ResetEPReadState();
			BCCIM_SendBroadcastPing(&DEVICE_CAN_Interface, CONTROL_CAN_Nodes, (pInt16U)&CONTROL_CAN_Nodes_Counter);
			break;

		default:
			return FALSE;
	}

	return TRUE;
}
// ----------------------------------------

void CONTROL_InitCAN()
{
	Int16U brp = CANA_BRP, tseg1 = CANA_TSEG1, tseg2 = CANA_TSEG2;
	if(DataTable[REG_CAN_BAUDRATE] == 1)
	{
		brp = CANA_BRP_100;
		tseg1 = CANA_TSEG1_100;
		tseg2 = CANA_TSEG2_100;
	}

	// Init CAN
	ZwCANa_Init(CANA_BR, brp, tseg1, tseg2, CANA_SJW);

	// Register system handler
	ZwCANa_RegisterSysEventHandler(&CONTROL_NotifyCANFault);

	// Allow interrupts for CAN
	ZwCANa_InitInterrupts(TRUE);
	ZwCANa_EnableInterrupts(TRUE);
}
// -----------------------------------------
