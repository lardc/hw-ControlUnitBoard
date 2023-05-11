// -----------------------------------------
// Global definitions
// ----------------------------------------

#ifndef __GLOBAL_H
#define __GLOBAL_H

// Include
#include "stdinc.h"

// Global parameters
//--------------------------------------------------------
// Password to unlock non-volatile area for write
#define ENABLE_LOCKING					FALSE
#define UNLOCK_PWD_1					1
#define UNLOCK_PWD_2					1
#define UNLOCK_PWD_3					1
#define UNLOCK_PWD_4					1
//
#define MME_CODE						0
#undef MME_PROTECTED_CALL
//
#define	SCCI_TIMEOUT_TICKS				1000
#define EP_COUNT						1
#define VALUES_x_SIZE					16
//--------------------------------------------------------

// Custom parameters
//--------------------------------------------------------
#define PATCH_STOP_BUTTON_DISABLE		TRUE
#define PATCH_INVERT_SEN0_INPUT			TRUE
#define PATCH_INVERT_SEN1_INPUT			TRUE
//--------------------------------------------------------

#endif // __GLOBAL_H
