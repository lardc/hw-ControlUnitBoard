// -----------------------------------------
// SCCI communication interface
// ----------------------------------------

#ifndef __SCCI_H
#define __SCCI_H

// Include
#include "stdinc.h"
//
#include "ZwDSP.h"
#include "SCCIParams.h"
#include "xCCICommon.h"

// Types
//
// Pointers for IO functions
typedef void (*SCCI_FUNC_SendArray16)(pInt16U Buffer, Int16U BufferSize);
typedef void (*SCCI_FUNC_ReceiveArray16)(pInt16U Buffer, Int16U BufferSize);
typedef Int16U (*SCCI_FUNC_GetBytesToReceive)();
typedef Int16U (*SCCI_FUNC_ReceiveByte)();
//
// IO configuration
typedef struct __SCCI_IOConfig
{
	SCCI_FUNC_SendArray16 IO_SendArray16;		
	SCCI_FUNC_ReceiveArray16 IO_ReceiveArray16;	
	SCCI_FUNC_GetBytesToReceive IO_GetBytesToReceive;
	SCCI_FUNC_ReceiveByte IO_ReceiveByte;
} SCCI_IOConfig, *pSCCI_IOConfig;
//
// Finite state-machine states 
typedef enum __SCCI_States
{
	SCCI_STATE_WAIT_STARTBYTE = 0,
	SCCI_STATE_WAIT_NID,
	SCCI_STATE_WAIT_HEADER,
	SCCI_STATE_WAIT_DUMMY,
	SCCI_STATE_WAIT_BODY
} SCCI_States;
//
// SCCI instance state
typedef struct __SCCI_Interface
{
	pSCCI_IOConfig IOConfig;
	pxCCI_ServiceConfig ServiceConfig;
	SCCI_States State;
	Int16U ExpectedBodyLength;
	Int16U DispID;
	Int16U MessageBuffer[xCCI_BUFFER_SIZE];
	pInt16U DataTableAddress;
	Int16U DataTableSize;
	Int32U TimeoutValueTicks;
	Int64U LastTimestampTicks;
	void *ArgForEPCallback;
	xCCI_ProtectionAndEndpoints ProtectionAndEndpoints;
} SCCI_Interface, *pSCCI_Interface;


// Functions
//
// Init interface instance
void SCCI_Init(pSCCI_Interface Interface, pSCCI_IOConfig IOConfig, pxCCI_ServiceConfig ServiceConfig,
 			   pInt16U DataTable, Int16U DataTableSize, Int32U MessageTimeoutTicks,
 			   void *ArgumentForCallback);
// Process packets
void SCCI_Process(pSCCI_Interface Interface, Int64U CurrentTickCount, Boolean MaskStateChangeOperations);
//
void SCCI_AnswerRead16(pSCCI_Interface Interface, Int16U Node, Int16U Address, Int16U Value);
void SCCI_AnswerRead16Double(pSCCI_Interface Interface, Int16U Node, Int16U Address1, Int16U Value1, Int16U Address2, Int16U Value2);
void SCCI_AnswerReadFloat(pSCCI_Interface Interface, Int16U Node, Int16U Address, Int32U Value);
void SCCI_AnswerRead32(pSCCI_Interface Interface, Int16U Node, Int16U Address, Int32U Value);
void SCCI_AnswerWrite16(pSCCI_Interface Interface, Int16U Node, Int16U Address);
void SCCI_AnswerWrite16Double(pSCCI_Interface Interface, Int16U Node, Int16U Address1, Int16U Address2);
void SCCI_AnswerWriteBlock16(pSCCI_Interface Interface, Int16U Node, Int16U Endpoint);
void SCCI_AnswerWrite32(pSCCI_Interface Interface, Int16U Node, Int16U Address);
void SCCI_AnswerWriteFloat(pSCCI_Interface Interface, Int16U Node, Int16U Address);
void SCCI_AnswerCall(pSCCI_Interface Interface, Int16U Node, Int16U Action);
void SCCI_AnswerReadBlock16Fast(pSCCI_Interface Interface, Int16U Node, Int16U Endpoint, pInt16U Data, Int16U DataSize);
void SCCI_AnswerError(pSCCI_Interface Interface, Int16U Node, Int16U ErrorCode, Int16U Details);

// Inline functions
//
// Create protected area
Int16U inline SCCI_AddProtectedArea(pSCCI_Interface Interface, Int16U StartAddress, Int16U EndAddress)
{
	return xCCI_AddProtectedArea(&(Interface->ProtectionAndEndpoints), StartAddress, EndAddress);
}
//
// Remove protected area
Boolean inline SCCI_RemoveProtectedArea(pSCCI_Interface Interface, Int16U AreaIndex)
{
	return xCCI_RemoveProtectedArea(&(Interface->ProtectionAndEndpoints), AreaIndex);
}
//
// Register read endpoint 16-bit source callback
Boolean inline SCCI_RegisterReadEndpoint16(pSCCI_Interface Interface, Int16U Endpoint,
								    xCCI_FUNC_CallbackReadEndpoint16 ReadCallback)
{
	return xCCI_RegisterReadEndpoint16(&(Interface->ProtectionAndEndpoints), Endpoint, ReadCallback);
}
//
// Register read endpoint 32-bit source callback
Boolean inline SCCI_RegisterReadEndpoint32(pSCCI_Interface Interface, Int16U Endpoint,
								    xCCI_FUNC_CallbackReadEndpoint32 ReadCallback)
{
	return xCCI_RegisterReadEndpoint32(&(Interface->ProtectionAndEndpoints), Endpoint, ReadCallback);
}
//
// Register write endpoint 16-bit destination callback
Boolean inline SCCI_RegisterWriteEndpoint16(pSCCI_Interface Interface, Int16U Endpoint,
									 xCCI_FUNC_CallbackWriteEndpoint16 WriteCallback)
{
	return xCCI_RegisterWriteEndpoint16(&(Interface->ProtectionAndEndpoints), Endpoint, WriteCallback);
}
//
// Register write endpoint 32-bit destination callback
Boolean inline SCCI_RegisterWriteEndpoint32(pSCCI_Interface Interface, Int16U Endpoint,
									 xCCI_FUNC_CallbackWriteEndpoint32 WriteCallback)
{
	return xCCI_RegisterWriteEndpoint32(&(Interface->ProtectionAndEndpoints), Endpoint, WriteCallback);
}


#endif // __SCCI_H
