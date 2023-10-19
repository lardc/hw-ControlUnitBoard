// -----------------------------------------
// Board-specific GPIO functions
// ----------------------------------------

#ifndef __ZBGPIO_H
#define __ZBGPIO_H

// Include
#include "stdinc.h"
#include "ZwDSP.h"

// Variables
//
extern Boolean Sensor1;
extern Boolean Sensor2;
extern Boolean Sensor3;
extern Boolean Sensor4;

// Functions
//
// Init GPIO and GPIO-PWM
void ZbGPIO_Init();
// Toggle led pin
void ZbGPIO_ToggleLedPin();
// Get pressed sensor
void ZbGPIO_ReadSensors();
void ZbGPIO_SwitchLamp1(Boolean Set);
void ZbGPIO_SwitchLamp2(Boolean Set);

#endif // __ZBGPIO_H
