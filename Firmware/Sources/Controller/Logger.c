// Header
#include "Logger.h"

// Includes
#include "Controller.h"
#include "DataTable.h"
#include "DeviceObjectDictionary.h"
#include "DeviceProfile.h"

// Functions
void LOG_SaveValues(pInt16U Data, Int16U DataCount)
{
	while(DataCount)
	{
		CONTROL_Values_1[CONTROL_Values_1_Counter++] = *Data++;
		DataCount--;
		if(CONTROL_Values_1_Counter >= VALUES_x_SIZE)
			CONTROL_Values_1_Counter = 0;
	}
	DEVPROFILE_ResetEPReadState();
	DataTable[REG_DIAG_ARRAY_LAST_VAL] = CONTROL_Values_1_Counter ? (CONTROL_Values_1_Counter - 1) : (VALUES_x_SIZE - 1);
}
// ----------------------------------------
