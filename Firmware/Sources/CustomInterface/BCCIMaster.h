// -----------------------------------------
// BCCI master communication interface
// ----------------------------------------

#ifndef __BCCI_MASTER_H
#define __BCCI_MASTER_H

// Include
#include "stdinc.h"
//
#include "BCCITypes.h"
#include "SCCI.h"

// Variables
extern Int16U	BCCIM_ReadBlock16Buffer[];
extern pInt32U	BCCIM_ReadBlockFloatBuffer;
extern Int16U	BCCIM_ReadBlockBufferCounter;

// Types
//
// BCCI instance state
typedef struct __BCCIM_Interface
{
	pBCCI_IOConfig IOConfig;
	Int32U TimeoutValueTicks;
	volatile Int64U *pTimerCounter;
} BCCIM_Interface, *pBCCIM_Interface;


// Functions
//
void BCCIM_Init(pBCCIM_Interface Interface, pBCCI_IOConfig IOConfig, Int32U MessageTimeoutTicks, volatile Int64U *pTimer);
void BCCIM_Process(pBCCIM_Interface Interface);
//
Int16U BCCIM_Read16(pBCCIM_Interface Interface, Int16U Node, Int16U Address, pInt16U Data);
Int16U BCCIM_Read32(pBCCIM_Interface Interface, Int16U Node, Int16U Address, pInt32U Data);
Int16U BCCIM_ReadFloat(pBCCIM_Interface Interface, Int16U Node, Int16U Address, pInt32U Data);
Int16U BCCIM_Write16(pBCCIM_Interface Interface, Int16U Node, Int16U Address, Int16U Data);
Int16U BCCIM_WriteBlock16(pBCCIM_Interface Interface, Int16U Node, Int16U Endpoint, pInt16U Data, Int16U DataLength);
Int16U BCCIM_Write32(pBCCIM_Interface Interface, Int16U Node, Int16U Address, Int32U Data);
Int16U BCCIM_WriteFloat(pBCCIM_Interface Interface, Int16U Node, Int16U Address, Int32U Data);
Int16U BCCIM_Call(pBCCIM_Interface Interface, Int16U Node, Int16U Action);
Int16U BCCIM_ReadBlock16(pBCCIM_Interface Interface, Int16U Node, Int16U Endpoint);
Int16U BCCIM_ReadBlockFloat(pBCCIM_Interface Interface, Int16U Node, Int16U Endpoint);

Int16U BCCIM_GetSavedErrorDetails();

#endif // __BCCI_MASTER_H
