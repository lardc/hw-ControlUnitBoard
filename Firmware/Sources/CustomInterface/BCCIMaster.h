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
// Set active scci interface
void BCCIM_SetActiveSCCI(pSCCI_Interface Interface);
// Init interface instance
void BCCIM_Init(pBCCIM_Interface Interface, pBCCI_IOConfig IOConfig, Int32U MessageTimeoutTicks, volatile Int64U *pTimer);
// Process packets
void BCCIM_Process(pBCCIM_Interface Interface);
//
// Read 16bit value
void BCCIM_Read16(pBCCIM_Interface Interface, Int16U Node, Int16U Address);
// Read double 16bit value
void BCCIM_Read16Double(pBCCIM_Interface Interface, Int16U Node, Int16U Address1, Int16U Address2);
// Read 16bit value
void BCCIM_Read32(pBCCIM_Interface Interface, Int16U Node, Int16U Address);
// Write 16bit value
void BCCIM_Write16(pBCCIM_Interface Interface, Int16U Node, Int16U Address, Int16U Data);
// Write block 16bit value
void BCCIM_WriteBlock16(pBCCIM_Interface Interface, Int16U Node, Int16U Endpoint, pInt16U Data, Int16U DataLength);
// Write double 16bit value
void BCCIM_Write16Double(pBCCIM_Interface Interface, Int16U Node, Int16U Address1, Int16U Data1, Int16U Address2, Int16U Data2);
// Write 32bit value
void BCCIM_Write32(pBCCIM_Interface Interface, Int16U Node, Int16U Address, Int32U Data);
// Call function
void BCCIM_Call(pBCCIM_Interface Interface, Int16U Node, Int16U Action);
// Read block 16bit value
void BCCIM_ReadBlock16(pBCCIM_Interface Interface, Int16U Node, Int16U Endpoint, Boolean Start);


#endif // __BCCI_MASTER_H
