// -----------------------------------------
// Parameters of BCCI interface
// ----------------------------------------

#ifndef __BCCI_PARAMS_H
#define __BCCI_PARAMS_H

#include "xCCIParams.h"

// Constants
//
#define DEVICE_CAN_ADDRESS			0

#define CAN_ACCEPTANCE_MASK			0x0003FC00
#define DEV_ADDR_MPY				(1ul << 10)

#define CAN_MASTER_NID_MASK			0x03FC0000		// Маска Master node ID
#define CAN_MASTER_NID_MPY			(1ul << 18)		// Смещение адреса Master

#define BR_TIMEOUT					100
#define SLAVE_COMPATIBLE_MODE

#endif // __BCCI_PARAMS_H
