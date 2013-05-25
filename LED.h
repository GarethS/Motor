/*
	Copyright (c) Gareth Scott 2013

	LED.h 

*/

#ifndef _LED_H_
#define _LED_H_

//#include "hw_ints.h"
//#include "hw_memmap.h"
//#include "hw_types.h"
//#include "gpio.h"
//#include "lmi_timer.h"

class LED {
public:
    LED() {On();}   // On when constructed
    ~LED() {Off();} // Off when destroyed

    void On(void) {GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0);}
    void Off(void) {GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, ~GPIO_PIN_0);}
};

#endif /* _LED_H_ */