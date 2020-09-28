﻿// Header
#include "StateMachine.h"

// Definitions
//
#define SM_TOU_ACT_MEASURE_START	100
//
#define SM_TOU_STATE_REGISTER		192
#define SM_TOU_STATE_IN_PROCESS		4
//
#define SM_OPRES_REGISTER			197
#define SM_OPRES_UNDEFINED			0

// Types
//
typedef enum __CUHV2Command
{
	CUHV2_EnablePower = 1,
	CUHV2_DisablePower = 2,
	CUHV2_ClearFault = 3,

	CUHV2_CommNone = 110,
	CUHV2_CommGate = 111,
	CUHV2_CommSL = 112,
	CUHV2_CommBVD = 113,
	CUHV2_CommBVR = 114,

	CUHV2_CommNoPE = 115,

	CUHV2_CommSLandGTU = 116,
	CUHV2_CommVgnt = 117

} CUHV2Command;

// Variables
//
static Boolean CUHV2Connected = TRUE, TOUConnected = FALSE;

// Functions
//
void SM_Reset()
{
	CUHV2Connected = TRUE;
	TOUConnected = FALSE;
}
// ----------------------------------------

Boolean SM_IsCUHV2Connected()
{
	return CUHV2Connected;
}
// ----------------------------------------

Boolean SM_IsCUHV2SwitchAction(Int16U Command)
{
	switch((CUHV2Command)Command)
	{
		case CUHV2_EnablePower:
		case CUHV2_DisablePower:
		case CUHV2_ClearFault:
		case CUHV2_CommNone:
		case CUHV2_CommGate:
		case CUHV2_CommSL:
		case CUHV2_CommBVD:
		case CUHV2_CommBVR:
		case CUHV2_CommSLandGTU:
		case CUHV2_CommVgnt:
			return TRUE;

		default:
			return FALSE;
	}
}
// ----------------------------------------

void SM_ProcessCUHV2Command(Int16U Command)
{
	switch((CUHV2Command)Command)
	{
		case CUHV2_EnablePower:
		case CUHV2_DisablePower:
		case CUHV2_ClearFault:
		case CUHV2_CommNone:
		case CUHV2_CommGate:
		case CUHV2_CommSL:
		case CUHV2_CommBVD:
		case CUHV2_CommBVR:
		case CUHV2_CommSLandGTU:
		case CUHV2_CommVgnt:
			CUHV2Connected = TRUE;
			break;

		case CUHV2_CommNoPE:
			CUHV2Connected = FALSE;
			break;
	}
}
// ----------------------------------------

Boolean SM_IsTOUConnected()
{
	return TOUConnected;
}
// ----------------------------------------

Boolean SM_IsTOUSwitchAction(Int16U Command)
{
	return (Command == SM_TOU_ACT_MEASURE_START);
}
// ----------------------------------------

void SM_ProcessTOUCommand(Int16U Command)
{
	if(Command == SM_TOU_ACT_MEASURE_START)
		TOUConnected = TRUE;
}
// ----------------------------------------

void SM_ProcessTOURegisterRead(Int16U Register, Int16U Data)
{
	if((Register == SM_TOU_STATE_REGISTER) && (Data != SM_TOU_STATE_IN_PROCESS) ||
		(Register == SM_OPRES_REGISTER) && (Data != SM_OPRES_UNDEFINED))
	{
		TOUConnected = FALSE;
	}
}
// ----------------------------------------
