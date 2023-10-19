// -----------------------------------------
// Logic controller
// ----------------------------------------

#ifndef __CONTROLLER_H
#define __CONTROLLER_H

// Include
#include "stdinc.h"
//
#include "ZwDSP.h"
#include "Global.h"


// Constants
typedef enum __DeviceState
{
	DS_None			= 0
} DeviceState;


// Variables
//
extern volatile Int64U CONTROL_TimeCounter;
extern volatile DeviceState CONTROL_State;
//
extern Int16U CONTROL_Values_1[VALUES_x_SIZE];
extern volatile Int16U CONTROL_Values_1_Counter;
extern volatile Int16U CONTROL_BootLoaderRequest;


// Functions
//
// Initialize controller
void CONTROL_Init();
// Update low-priority states
void CONTROL_Idle();
// Notify that CAN system fault occurs
void CONTROL_NotifyCANFault(ZwCAN_SysFlags Flag);


#endif // __CONTROLLER_H
