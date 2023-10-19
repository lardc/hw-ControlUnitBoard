// -----------------------------------------
// Driver for EEPROM & FRAM via SPI
// ----------------------------------------

#ifndef __ZBMEMORY_H
#define __ZBMEMORY_H

// Include
#include "stdinc.h"
#include "ZwDSP.h"

// Functions
//
// Write values to EPROM
void ZbMemory_WriteValuesEPROM(Int16U EPROMAddress, pInt16U Buffer, Int16U BufferSize);
// Read values from EPROM
void ZbMemory_ReadValuesEPROM(Int16U EPROMAddress, pInt16U Buffer, Int16U BufferSize);

#endif // __ZBMEMORY_H
