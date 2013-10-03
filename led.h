/*
	Copyright (c) Gareth Scott 2013

	led.h 

*/

#ifndef _LED_H_
#define _LED_H_

//#include "hw_ints.h"
//#include "hw_memmap.h"
//#include "hw_types.h"
//#include "gpio.h"
//#include "lmi_timer.h"
#include "compilerHelper.h" // Contains TRUE, FALSE definitions

#if CYGWIN
#include "log.h"
#include <sstream>

using namespace std;
#endif /* CYGWIN */

// TODO - need to specify which LED. Currently only one on board.
class led
#if CYGWIN
    : public logc
#endif /* CYGWIN */
{
public:
    led(bool onAtConstruction = TRUE)
#if CYGWIN 
        : logc(std::string("LED"))
#endif /* CYGWIN */					
    {if (onAtConstruction) {On();} else {Off();}}
    ~led() {Off();} // Off when destroyed

    void set(const unsigned int onOrOff) {if (onOrOff) {On();} else {Off();}}
#if CYGWIN
    void On(void) {oss() << "LED on "; dump();}
    void Off(void) {oss() << "LED off "; dump();}
#else // not CYGWIN 
    // Trying to access a GPIO port before it is configured results in the hard fault handler being called.
#ifdef PART_TM4C1233D5PM
    void On(void) {if (!enable()){return;} GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_1, GPIO_PIN_1);}
    void Off(void) {if (!enable()){return;} GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_1, ~GPIO_PIN_1);}
#else // not PART_TM4C1233D5PM    
    void On(void) {if (!enable()){return;} GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0);}
    void Off(void) {if (!enable()){return;} GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, ~GPIO_PIN_0);}
#endif // PART_TM4C1233D5PM    
    static bool enable(void) {return _enable;}
    static void enable(bool e) {_enable = e;}
    
private:
    static bool _enable;    
#endif // CYGWIN    
};

#endif /* _LED_H_ */