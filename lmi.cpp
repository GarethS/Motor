/*
	Copyright (c) Gareth Scott 2011

	lmi.cpp

	luminary micro inc. 
	This file enables building under CYGWIN without having to #ifdef out all the Luminary Micro specific library functions.
	
*/

#include "hw_types.h"
#include "lmi_timer.h"
#include "gpio.h"

void TimerLoadSet(unsigned long ulBase, unsigned long ulTimer, unsigned long ulValue) {}
void TimerEnable(unsigned long ulBase, unsigned long ulTimer) {}
void TimerDisable(unsigned long ulBase, unsigned long ulTimer) {}

void GPIODirModeSet(unsigned long ulPort, unsigned char ucPins, unsigned long ulPinIO) {}
void GPIOPadConfigSet(unsigned long ulPort, unsigned char ucPins, unsigned long ulStrength, unsigned long ulPadType) {}
void GPIOPinWrite(unsigned long ulPort, unsigned char ucPins, unsigned char ucVal) {}


