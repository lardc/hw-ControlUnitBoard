// -----------------------------------------
// Board-specific GPIO functions
// ----------------------------------------

// Header
#include "ZbGPIO.h"

// Include
#include "SysConfig.h"
#include "DeviceObjectDictionary.h"
#include "DataTable.h"

#define REFdebounce	50

Int16U 	sCounter1_0 = 0, sCounter1_1 = 0,
		sCounter2_0 = 0, sCounter2_1 = 0,
		sCounter3_0 = 0, sCounter3_1 = 0,
		sCounter4_0 = 0, sCounter4_1 = 0;

Boolean Sensor1 = FALSE, Sensor2 = FALSE, Sensor3 = FALSE, Sensor4 = FALSE;

static Int16U VAR_PIN_OPTO_SW = PIN_OPTO_SW,
		VAR_PIN_LAMP_0 = PIN_LAMP_0, VAR_PIN_LAMP_1 = PIN_LAMP_1;

// Functions
//
void ZbGPIO_Init()
{
	if(DataTable[REG_PCB1_1])
	{
		VAR_PIN_OPTO_SW = PIN_OPTO_SW_V11;
		VAR_PIN_LAMP_0 = PIN_LAMP_0_V11;
		VAR_PIN_LAMP_1 = PIN_LAMP_1_V11;
	}

   	ZwGPIO_WritePin(PIN_LED, FALSE);
   	ZwGPIO_PinToOutput(PIN_LED);

   	ZwGPIO_WritePin(VAR_PIN_OPTO_SW, FALSE);
   	ZwGPIO_PinToOutput(VAR_PIN_OPTO_SW);

   	ZwGPIO_WritePin(VAR_PIN_LAMP_0, FALSE);
   	ZwGPIO_WritePin(VAR_PIN_LAMP_1, FALSE);
   	ZwGPIO_PinToOutput(VAR_PIN_LAMP_0);
   	ZwGPIO_PinToOutput(VAR_PIN_LAMP_1);

   	ZwGPIO_PinToInput(PIN_SENS_0, TRUE, 0);
   	ZwGPIO_PinToInput(PIN_SENS_1, TRUE, 0);
   	ZwGPIO_PinToInput(PIN_SENS_2, TRUE, 0);
   	ZwGPIO_PinToInput(PIN_SENS_3, TRUE, 0);
}
// ----------------------------------------

void ZbGPIO_ToggleLedPin()
{
	ZwGPIO_TogglePin(PIN_LED);
}
// ----------------------------------------

void ZbGPIO_TurnOnPC()
{
	pInt16U TurnFlagPointer = (pInt16U)0x3FE;
	const Int16U TurnFlagValue = 0xA5A5;

	// Включение ПК только при отсутствии выставленного в памяти флага
	if(TurnFlagValue != *TurnFlagPointer)
	{
		*TurnFlagPointer = TurnFlagValue;

		ZwGPIO_WritePin(VAR_PIN_OPTO_SW, TRUE);
		DELAY_US(500000);
		ZwGPIO_WritePin(VAR_PIN_OPTO_SW, FALSE);
	}
}
// ----------------------------------------

void ZbGPIO_SwitchLamp1(Boolean Set)
{
	ZwGPIO_WritePin(VAR_PIN_LAMP_0, Set);
}
// ----------------------------------------

void ZbGPIO_SwitchLamp2(Boolean Set)
{
	ZwGPIO_WritePin(VAR_PIN_LAMP_1, Set);
}
// ----------------------------------------

void ZbGPIO_ReadSensors()
{
	if(ZwGPIO_ReadPin(PIN_SENS_0))
	{
		sCounter1_0 = 0;
		sCounter1_1++;

		if(sCounter1_1 >= REFdebounce)
		{
			sCounter1_1 = REFdebounce + 1;
			Sensor1 = FALSE;
		}
	}
	else
	{
		sCounter1_0++;
		sCounter1_1 = 0;

		if(sCounter1_0 >= REFdebounce)
		{
			sCounter1_0 = REFdebounce + 1;
			Sensor1 = TRUE;
		}
	}

	if(ZwGPIO_ReadPin(PIN_SENS_1))
	{
		sCounter2_0 = 0;
		sCounter2_1++;

		if(sCounter2_1 >= REFdebounce)
		{
			sCounter2_1 = REFdebounce + 1;
			Sensor2 = FALSE;
		}
	}
	else
	{
		sCounter2_0++;
		sCounter2_1 = 0;

		if(sCounter2_0 >= REFdebounce)
		{
			sCounter2_0 = REFdebounce + 1;
			Sensor2 = TRUE;
		}
	}

	if(ZwGPIO_ReadPin(PIN_SENS_2))
	{
		sCounter3_0 = 0;
		sCounter3_1++;

		if(sCounter3_1 >= REFdebounce)
		{
			sCounter3_1 = REFdebounce + 1;
			Sensor3 = FALSE;
		}
	}
	else
	{
		sCounter3_0++;
		sCounter3_1 = 0;

		if(sCounter3_0 >= REFdebounce)
		{
			sCounter3_0 = REFdebounce + 1;
			Sensor3 = TRUE;
		}
	}

	if(ZwGPIO_ReadPin(PIN_SENS_3))
	{
		sCounter4_0 = 0;
		sCounter4_1++;

		if(sCounter4_1 >= REFdebounce)
		{
			sCounter4_1 = REFdebounce + 1;
			Sensor4 = FALSE;
		}
	}
	else
	{
		sCounter4_0++;
		sCounter4_1 = 0;

		if(sCounter4_0 >= REFdebounce)
		{
			sCounter4_0 = REFdebounce + 1;
			Sensor4 = TRUE;
		}
	}
}
// ----------------------------------------
