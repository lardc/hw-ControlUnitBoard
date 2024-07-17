// -----------------------------------------
// System parameters
// ----------------------------------------
#ifndef __SYSCONFIG_H
#define __SYSCONFIG_H

// Include
#include <ZwBase.h>
#include <BoardConfig.h>

// CPU & System
//--------------------------------------------------------
#define CPU_PLL				10          // OSCCLK * PLL div 2 = CPUCLK: 20 * 10 / 2 = 100
#define CPU_CLKINDIV		0           // "div 2" in previous equation
#define SYS_HISPCP       	0x01   		// SYSCLKOUT / 2
#define SYS_LOSPCP       	0x02    	// SYSCLKOUT / 4
//--------------------------------------------------------

// Boot-loader
//--------------------------------------------------------
#define BOOT_LOADER_REQUEST	0xABCD
//--------------------------------------------------------

// Power control
//--------------------------------------------------------
#define SYS_PUMOD			ZW_POWER_SCIA_CLK | ZW_POWER_SCIB_CLK | ZW_POWER_CANA_CLK

#define SYS_WD_PRESCALER	0x07
//--------------------------------------------------------

// GPIO
//--------------------------------------------------------
// Input filters
#define GPIO_TSAMPLE		50		// T[sample_A] = (1/ 100MHz) * (2 * 50) = 1 uS
#define GPIO_NSAMPLE		6		// 6 samples: T = 1uS * 6 = 6 uS

// Flash
//--------------------------------------------------------
#define FLASH_FWAIT			3
#define FLASH_OTPWAIT		5
//--------------------------------------------------------

// TIMERs
//--------------------------------------------------------
#define TIMER2_FREQ			1000	// in KHz
#define TIMER2_PERIOD		(1000000L / TIMER2_FREQ)

#define DBG_FREQ			2		// 2 Hz
#define DBG_COUNTER_PERIOD	(1000 / (DBG_FREQ * 2))
//--------------------------------------------------------

// SCI-A
//--------------------------------------------------------
#define SCIA_BR				115200L		// UART baudrate = 115200 bps
#define SCIA_DB				8
#define SCIA_SB				FALSE
#define SCIA_PARITY			ZW_PAR_NONE

// SCI-B
//--------------------------------------------------------
#define SCIB_BR				115200L		// UART baudrate = 115200 bps
#define SCIB_DB				8
#define SCIB_SB				FALSE
#define SCIB_PARITY			ZW_PAR_NONE
//--------------------------------------------------------

// CAN-A
//--------------------------------------------------------
#define CANA_BR				1000000L
#define CANA_BRP			9
#define CANA_TSEG1			6
#define CANA_TSEG2			1
#define CANA_SJW			1
//--------------------------------------------------------

#endif // __SYSCONFIG_H
