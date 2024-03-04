// -----------------------------------------
// Program entry point
// ----------------------------------------

// Include
#include <stdinc.h>
//
#include "ZwDSP.h"
#include "ZbBoard.h"
//
#include "SysConfig.h"
//
#include "Controller.h"
#include "DeviceObjectDictionary.h"
#include "DataTable.h"
#include "DeviceProfile.h"
#include "Global.h"
#include "Flash.h"

// FORWARD FUNCTIONS
Boolean InitializeCPU();
void InitializeTimers();
void InitializeSCI();
void InitializeCAN();

// FORWARD ISRs
// CPU Timer 2 ISR
ISRCALL Timer2_ISR();
// CAN Line 0 ISR
ISRCALL CAN0_ISR();

// FUNCTIONS
void main()
{
	InitializeCPU();
	InitializeTimers();
	InitializeCAN();
	InitializeSCI();

	// Инициализация EEPROM, контроллера и GPIO (с учётом настроек в DT)
	FLASH_Init();
	CONTROL_Init();

	ZbGPIO_Init();
	ZbGPIO_TurnOnPC();

	// Setup ISRs
	BEGIN_ISR_MAP
		ADD_ISR(TINT2, Timer2_ISR);
		ADD_ISR(ECAN0INTA, CAN0_ISR);
	END_ISR_MAP

	// Init board external watch-dog
	ZbWatchDog_Init();

	// Enable interrupts
	EINT;
	ERTM;

	// Set watch-dog as WDRST
	ZwSystem_SelectDogFunc(FALSE);
	// Enable watch-dog
	ZwSystem_EnableDog(SYS_WD_PRESCALER);

	// Start system ticks timer
	ZwTimer_StartT2();

	// Background cycle
	while(TRUE)
		CONTROL_Idle();
}
// -----------------------------------------

// Initialize and prepare DSP
Boolean InitializeCPU()
{
	Boolean clockInitResult;

	// Init clock and peripherals
	clockInitResult = ZwSystem_Init(CPU_PLL, CPU_CLKINDIV, SYS_LOSPCP, SYS_HISPCP, SYS_PUMOD);

	if(clockInitResult)
	{
		// Do default GPIO configuration
		ZwGPIO_Init(GPIO_TSAMPLE, GPIO_TSAMPLE, GPIO_TSAMPLE, GPIO_TSAMPLE, GPIO_TSAMPLE);
		// Initialize PIE
		ZwPIE_Init();
		// Prepare PIE vectors
		ZwPIE_Prepare();
	}

	// Config flash
	ZW_FLASH_CODE_SHADOW;
	ZW_FLASH_OPTIMIZE(FLASH_FWAIT, FLASH_OTPWAIT);

	return clockInitResult;
}
// -----------------------------------------

// Initialize CPU timers
void InitializeTimers()
{
	ZwTimer_InitT2();
	ZwTimer_SetT2(TIMER2_PERIOD);
	ZwTimer_EnableInterruptsT2(TRUE);
}
// -----------------------------------------

void InitializeSCI()
{
	// Initialize and prepare SCI modules
	ZwSCIa_Init(SCIA_BR, SCIA_DB, SCIA_PARITY, SCIA_SB, FALSE);
	ZwSCIa_InitFIFO(16, 0);
	ZwSCIa_EnableInterrupts(FALSE, FALSE);

	ZwSCIb_Init(SCIB_BR, SCIB_DB, SCIB_PARITY, SCIB_SB, FALSE);
	ZwSCIb_InitFIFO(16, 0);
	ZwSCIb_EnableInterrupts(FALSE, FALSE);

	ZwSCI_EnableInterruptsGlobal(FALSE);
}
// -----------------------------------------

void InitializeCAN()
{
	// Init CAN
	ZwCANa_Init(CANA_BR, CANA_BRP, CANA_TSEG1, CANA_TSEG2, CANA_SJW);

	// Register system handler
	ZwCANa_RegisterSysEventHandler(&CONTROL_NotifyCANFault);

	// Allow interrupts for CAN
	ZwCANa_InitInterrupts(TRUE);
	ZwCANa_EnableInterrupts(TRUE);
}
// -----------------------------------------

void InitializeBoard()
{
	// Init board GPIO
	ZbGPIO_Init();
}
// -----------------------------------------

// ISRs
// -----------------------------------------
#ifdef BOOT_FROM_FLASH
	#pragma CODE_SECTION(Timer2_ISR, "ramfuncs");
	#pragma CODE_SECTION(CAN0_ISR, "ramfuncs");
#endif
//
#pragma INTERRUPT(Timer0_ISR, HPI);

ISRCALL Timer2_ISR()
{
	static Int16U dbgCounter = 0;
	CONTROL_TimeCounter++;

	// Service watch-dogs
	if (CONTROL_BootLoaderRequest != BOOT_LOADER_REQUEST)
	{
		ZwSystem_ServiceDog();
		ZbWatchDog_Strobe();
	}

	// Моргание светодиодом
	++dbgCounter;
	if(dbgCounter == DBG_COUNTER_PERIOD)
	{
		ZbGPIO_ToggleLedPin();
		dbgCounter = 0;
	}

	// Управление лампами и датчиками
	ZbGPIO_SwitchLamp1(DataTable[REG_LAMP_1]);
	ZbGPIO_SwitchLamp2(DataTable[REG_LAMP_2]);

	ZbGPIO_ReadSensors();
	DataTable[REG_SENSOR_1] = (DataTable[REG_INVERT_SEN1] ? !Sensor1 : Sensor1) ? 1 : 0;
	DataTable[REG_SENSOR_2] = (DataTable[REG_INVERT_SEN2] ? !Sensor2 : Sensor2) ? 1 : 0;
	DataTable[REG_SENSOR_3] = (DataTable[REG_INVERT_SEN3] ? !Sensor3 : Sensor3) ? 1 : 0;
	DataTable[REG_SENSOR_4] = (DataTable[REG_INVERT_SEN4] ? !Sensor4 : Sensor4) ? 1 : 0;

	if(DataTable[REG_DISABLE_SEN4])
		DataTable[REG_SENSOR_4] = 0;

	// no PIE
	TIMER2_ISR_DONE;
}
// -----------------------------------------

// Line 0 ISR
ISRCALL CAN0_ISR()
{
	// handle CAN system events
	ZwCANa_DispatchSysEvent();

	// allow other interrupts from group 9
	CAN_ISR_DONE;
}
// -----------------------------------------
