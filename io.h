/*
	Copyright (c) Gareth Scott 2013

	io.h 

*/

#ifndef _IO_H_
#define _IO_H_

#include <stdbool.h>
#include <stdint.h>
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_types.h"
#if CYGWIN
#include "gpio.h"
#include "lmi_timer.h"
#else // not CYGWIN
#ifdef PART_TM4C1233D5PM
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/adc.h"
#include "driverlib/sysctl.h"
#else // not PART_TM4C1233D5PM
#include "gpio.h"
#include "timer.h"
#include "adc.h"
#endif // PART_TM4C1233D5PM
#endif // CYGWIN

#if CYGWIN
#include "log.h"
#include <sstream>

using namespace std;
#define PART_TM4C1233D5PM
#endif /* CYGWIN */

#ifdef PART_TM4C1233D5PM
#define PIN_0       (0x01)
#define PIN_1       (0x02)
#define PIN_2       (0x04)
#define PIN_3       (0x08)
#define PIN_4       (0x10)

#define PIN_IN_0    (GPIO_PIN_2)    // PORTA
#define PIN_IN_1    (GPIO_PIN_3)    // PORTA
#define PIN_IN_2    (GPIO_PIN_4)    // PORTA
#define PIN_IN_3    (GPIO_PIN_5)    // PORTA
#define PIN_IN_4    (GPIO_PIN_6)    // PORTA
#define PIN_IN_ALL  (PIN_IN_0 | PIN_IN_1 | PIN_IN_2 | PIN_IN_3 | PIN_IN_4)

#define PIN_OUT_0   (GPIO_PIN_0)    // PORTB
#define PIN_OUT_1   (GPIO_PIN_1)    // PORTB
#define PIN_OUT_2   (GPIO_PIN_2)    // PORTB
#define PIN_OUT_3   (GPIO_PIN_3)    // PORTB
#define PIN_OUT_4   (GPIO_PIN_6)    // PORTB
#define PIN_OUT_ALL (PIN_OUT_0 | PIN_OUT_1 | PIN_OUT_2 | PIN_OUT_3 | PIN_OUT_4)

#define PIN_ADC_0   (GPIO_PIN_4)    // PORTB, AIN10
#define PIN_ADC_1   (GPIO_PIN_5)    // PORTB, AIN11
#define PIN_ADC_2   (GPIO_PIN_0)    // PORTD, AIN7
#define PIN_ADC_3   (GPIO_PIN_1)    // PORTD, AIN6
#define PIN_ADC_4   (GPIO_PIN_2)    // PORTD, AIN5

#define GPIO_IN_PORT    (GPIO_PORTA_BASE)
#define GPIO_OUT_PORT   (GPIO_PORTB_BASE)

#define ADC_COUNT       (5)

#define ADC_SEQUENCE_0  (0) // Captures up to 8 samples
#define ADC_SEQUENCE_1  (1) // Captures up to 4 samples
#define ADC_SEQUENCE_2  (2) // Captures up to 4 samples
#define ADC_SEQUENCE_3  (3) // Captures up to 1 sample

#define ADC_PRIORITY_HIGHEST    (0)
#define ADC_PRIORITY_HIGH       (1)
#define ADC_PRIORITY_LOW        (2)
#define ADC_PRIORITY_LOWEST     (3)
#else // not PART_TM4C1233D5PM
#endif // PART_TM4C1233D5PM

class io
#if CYGWIN
				: public logc
#endif /* CYGWIN */
				{
public:
    io();
    ~io() {}

    unsigned int getInput(void) const;
    void setOutput(unsigned int out);
    unsigned int getADC(unsigned int adcIndex);
    int getTemperature(void);
    void reset(void) {
#if !CYGWIN        
        SysCtlReset();
#endif // not CYGWIN        
    }
    void init(void);
    
private:
    uint32_t _adcValue[ADC_COUNT];
    uint32_t _temperatureValue[1];

};

#endif /* _IO_H_ */