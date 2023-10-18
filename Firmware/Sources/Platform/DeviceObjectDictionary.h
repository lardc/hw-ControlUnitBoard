// -----------------------------------------
// Logic controller
// ----------------------------------------

#ifndef __DEV_OBJ_DIC_H
#define __DEV_OBJ_DIC_H

// REGISTERS
//
#define ACT_RESET_STATE_MACHINE		10	// Reset state machine to default state
//
#define ACT_SAVE_TO_ROM				200	// Save parameters to EEPROM module
#define ACT_RESTORE_FROM_ROM		201	// Restore parameters from EEPROM module
#define ACT_RESET_TO_DEFAULT		202	// Reset parameters to default values (only in controller memory)
//
#define ACT_BOOT_LOADER_REQUEST		320	// Request reboot to bootloader
//
#define REG_LAMP_1					128	//
#define REG_LAMP_2					129	//
#define REG_LAMP_3					130	//
//
#define REG_DEV_STATE				192	// Device state
#define REG_FAULT_REASON			193	// Fault reason in the case DeviceState -> FAULT
#define REG_DISABLE_REASON			194	// Fault reason in the case DeviceState -> DISABLED
#define REG_WARNING					195	// Warning if present
//
#define REG_SENSOR_1				197	//
#define REG_SENSOR_2				198	//
#define REG_SENSOR_3				199	//
#define REG_SENSOR_4				200	//
//
#define REG_OVERLAP_COUNT_REQ		210	// Overlapping requests via SCCI interface
#define REG_OVERLAP_COUNT_RESP		211	// Overlapping responses via SCCI interface
//
#define REG_CAN_BUSOFF_COUNTER		220 // Counter of bus-off states
#define REG_CAN_STATUS_REG			221	// CAN status register (32 bit)
#define REG_CAN_STATUS_REG_32		222
#define REG_CAN_DIAG_TEC			223	// CAN TEC
#define REG_CAN_DIAG_REC			224	// CAN REC
//
#define REG_MME_CODE				250	// MME code number
//
// ----------------------------------------
//
#define REG_FWINFO_SLAVE_NID		256	// Device CAN slave node ID
#define REG_FWINFO_MASTER_NID		257	// Device CAN master node ID (if presented)
// 258 - 259
#define REG_FWINFO_STR_LEN			260	// Length of the information string record
#define REG_FWINFO_STR_BEGIN		261	// Begining of the information string record


// ENDPOINTS
//
#define EP16_Data_1					1	// Data of 1

// PROBLEM CODES
//
#define PROBLEM_NONE				0

// FAULT CODES
//
#define FAULT_NONE					0	// No fault

// WARNING CODES
//
#define WARNING_NONE				0
//
#define WARNING_WATCHDOG_RESET		1001	// System has been reseted by WD

// DISABLE CODES
//
#define DISABLE_NONE				0
//
#define DISABLE_BAD_CLOCK			1001	// Problem with main oscillator

// DIAG CODES
//
#define DIAG_NO_ERROR				0

// USER ERROR CODES
//
#define ERR_NONE					0	// No error
#define ERR_WRONG_PWD				4	// Wrong password - unlock failed


#endif // __DEV_OBJ_DIC_H
