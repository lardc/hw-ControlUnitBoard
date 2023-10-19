// -----------------------------------------
// Board parameters
// ----------------------------------------

#ifndef __BOARD_CONFIG_H
#define __BOARD_CONFIG_H

// Include
#include <ZwBase.h>

// Program build mode
//
#define BOOT_FROM_FLASH					// normal mode

// Board options
#define DSP28_2809						// on-board CPU
#define OSC_FRQ				(20MHz)		// on-board oscillator
#define CPU_FRQ_MHZ			100			// CPU frequency = 100MHz
#define CPU_FRQ				CPU_FRQ_MHZ * 1000000L 
#define SYS_HSP_FREQ		CPU_FRQ / 2 // High-speed bus frequency
#define SYS_LSP_FREQ		CPU_FRQ / 4 // Low-speed bus frequency
#define ZW_PWM_DUTY_BASE	1024

// Peripheral options
#define HWUSE_SCI_A

// IO placement
//
#define SCI_A_QSEL			GPAQSEL2
#define SCI_A_MUX			GPAMUX2
#define SCI_A_TX			GPIO29
#define SCI_A_RX			GPIO28
#define SCI_A_MUX_SELECTOR	1

// Pins
#define PIN_OPTO_SW			23
#define PIN_LED				4
//
#define PIN_SENS_0			1
#define PIN_SENS_1			3
#define PIN_SENS_2			0
#define PIN_SENS_3			16
//
#define PIN_LAMP_0			15
#define PIN_LAMP_1			14
//
#define PIN_LAMP_0_V11		11
#define PIN_LAMP_1_V11		9
#define PIN_OPTO_SW_V11		10
//
#define PIN_WD_RST			32


#endif // __BOARD_CONFIG_H
