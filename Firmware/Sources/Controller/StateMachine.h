#ifndef __STATE_MACHINE_H
#define __STATE_MACHINE_H

// Include
#include "stdinc.h"
#include "ZwDSP.h"

// Defines
//
#define SM_CUHV2_NODE_ID			1
#define SM_TOU_NODE_ID				11

// Functions
//
Boolean SM_IsCUHV2Connected();
Boolean SM_IsCUHV2SwitchAction(Int16U Command);
void SM_ProcessCUHV2Command(Int16U Command);

Boolean SM_IsTOUConnected();
Boolean SM_IsTOUSwitchAction(Int16U Command);
void SM_ProcessTOUCommand(Int16U Command);
void SM_ProcessTOURegisterRead(Int16U Register, Int16U Data);

#endif // __STATE_MACHINE_H
