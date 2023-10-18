// ----------------------------------------
// Driver for MCU EEPROM
// ----------------------------------------

// Header
#include "ZbMemory.h"
//
#include "SysConfig.h"
#include "ZbGPIO.h"
#include "Flash.h"
#include "ZwSysCtrl.h"

// Definitions
#define FLASH_SECTOR			SECTORH
#define FLASH_START_ADDR		0x3D8000

// Functions
//
void ZbMemory_WriteValuesEPROM(Int16U EPROMAddress, pInt16U Buffer, Int16U BufferSize)
{
	ZwSystem_DisableDog();
	DINT;
	Status = Flash_Erase(FLASH_SECTOR, (FLASH_ST *)&FlashStatus);
	Status = Flash_Program((pInt16U)FLASH_START_ADDR, Buffer, BufferSize, (FLASH_ST *)&FlashStatus);
	EINT;
	ZwSystem_EnableDog(SYS_WD_PRESCALER);
}
// ----------------------------------------

void ZbMemory_ReadValuesEPROM(Int16U EPROMAddress, pInt16U Buffer, Int16U BufferSize)
{
	Int16U i;
	for(i = 0; i < BufferSize; i++)
		Buffer[i] = *(pInt16U)(FLASH_START_ADDR + i);
}
// ----------------------------------------
