// -----------------------------------------
// Board-specific GPIO functions
// ----------------------------------------

// Header
#include "ZbGPIO.h"

// Include
#include "SysConfig.h"

#define REFdebounce	50
Int16U 	sCounter1_0 = 0, sCounter1_1 = 0,
		sCounter2_0 = 0, sCounter2_1 = 0,
		sCounter3_0 = 0, sCounter3_1 = 0,
		sCounter4_0 = 0, sCounter4_1 = 0;

Boolean Sensor1 = FALSE, Sensor2 = FALSE, Sensor3 = FALSE, Sensor4 = FALSE;

// Functions
//
void ZbGPIO_Init()
{
   	ZwGPIO_WritePin(PIN_LED, FALSE);
   	ZwGPIO_WritePin(PIN_OPTO_SW, FALSE);
   	ZwGPIO_PinToOutput(PIN_LED);
   	ZwGPIO_PinToOutput(PIN_OPTO_SW);

   	ZwGPIO_WritePin(PIN_LAMP_0, FALSE);
   	ZwGPIO_WritePin(PIN_LAMP_1, FALSE);
   	ZwGPIO_PinToOutput(PIN_LAMP_0);
   	ZwGPIO_PinToOutput(PIN_LAMP_1);

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

void ZbGPIO_SwitchLamp1(Boolean Set)
{
	ZwGPIO_WritePin(PIN_LAMP_0, Set);
}
// ----------------------------------------

void ZbGPIO_SwitchLamp2(Boolean Set)
{
	ZwGPIO_WritePin(PIN_LAMP_1, Set);
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
