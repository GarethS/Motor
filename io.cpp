/*
	Copyright (c) Gareth Scott 2013, 2014

	io.cpp 

*/

#include "io.h"
#include <assert.h>

#if !CYGWIN
#include <stdio.h>
#include <string.h>

#ifdef PART_TM4C1233D5PM
#include "driverlib/sysctl.h"   // SYSCTL_PERIPH_GPIOC
#include "driverlib/rom.h"

#include "hw_nvic.h"
#include "driverlib/usb.h"

// The following 4 lines needed for: USBDCDCTerm()
#include "usblib/usblib.h"
#include "usblib/usbcdc.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdcdc.h"
#endif // PART_TM4C1233D5PM
#endif /* not CYGWIN */

io::io() 
#if CYGWIN 
        : logc(std::string("IO"))
#endif /* CYGWIN */					
				 {
}

void io::init(void) {
#if !CYGWIN
#define BOOTLOADER_TEST 0
#if BOOTLOADER_TEST
    //ROM_UpdateUART();
    // May need to do the following here:
    //  0. See if this will cause bootloader to start
    //ROM_FlashErase(0); 
    //ROM_UpdateUSB(0);
    
    //USBDCDTerm(0);
    
    // Disable all interrupts
    ROM_IntMasterDisable();
    ROM_SysTickIntDisable();
    ROM_SysTickDisable();
    HWREG(NVIC_DIS0) = 0xffffffff;
    HWREG(NVIC_DIS1) = 0xffffffff;
    
    //  1. Enable USB PLL
    //ROM_SysCtlUSBPLLEnable();
    //  2. Enable USB controller
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_USB0);
    ROM_SysCtlPeripheralReset(SYSCTL_PERIPH_USB0);
    //USBClockEnable(USB0_BASE, 8, USB_CLOCK_INTERNAL);
    //HWREG(USB0_BASE + USB_O_CC) = (8 - 1) | USB_CLOCK_INTERNAL;

    ROM_SysCtlUSBPLLEnable();
    
    //  3. Enable USB D+ D- pins

    //  4. Activate USB DFU
    ROM_SysCtlDelay(100000 /*ui32SysClock*/ / 3);
    ROM_IntMasterEnable();  // Re-enable interrupts at NVIC level
    ROM_UpdateUSB(0);
    //  5. Should never get here since update is in progress
#endif // BOOTLOADER_TEST
    
    // Inputs
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_GPIOPinTypeGPIOInput(GPIO_IN_PORT, PIN_IN_ALL);
    
    // Outputs
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    ROM_GPIOPinTypeGPIOOutput(GPIO_OUT_PORT, PIN_OUT_ALL);
    
    // ADCs
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);    // N.B. GPIOB is enabled just a few lines above
    ROM_GPIOPinTypeADC(GPIO_PORTB_BASE, PIN_ADC_0 | PIN_ADC_1);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_GPIOPinTypeADC(GPIO_PORTD_BASE, PIN_ADC_2 | PIN_ADC_3 | PIN_ADC_4);
    
    ROM_ADCSequenceConfigure(ADC0_BASE, ADC_SEQUENCE_0, ADC_TRIGGER_PROCESSOR, ADC_PRIORITY_HIGHEST);
    ROM_ADCSequenceStepConfigure(ADC0_BASE, ADC_SEQUENCE_0, 0, ADC_CTL_CH10 | ADC_CTL_IE);
    ROM_ADCSequenceStepConfigure(ADC0_BASE, ADC_SEQUENCE_0, 1, ADC_CTL_CH11 | ADC_CTL_IE);
    ROM_ADCSequenceStepConfigure(ADC0_BASE, ADC_SEQUENCE_0, 2, ADC_CTL_CH7 | ADC_CTL_IE);
    ROM_ADCSequenceStepConfigure(ADC0_BASE, ADC_SEQUENCE_0, 3, ADC_CTL_CH6 | ADC_CTL_IE);
    ROM_ADCSequenceStepConfigure(ADC0_BASE, ADC_SEQUENCE_0, 4, ADC_CTL_CH5 | ADC_CTL_IE | ADC_CTL_END);
    ROM_ADCSequenceEnable(ADC0_BASE, ADC_SEQUENCE_0);
    ROM_ADCIntClear(ADC0_BASE, ADC_SEQUENCE_0);

    ROM_ADCSequenceConfigure(ADC0_BASE, ADC_SEQUENCE_3, ADC_TRIGGER_PROCESSOR, ADC_PRIORITY_HIGHEST);
    ROM_ADCSequenceStepConfigure(ADC0_BASE, ADC_SEQUENCE_3, 0, ADC_CTL_TS | ADC_CTL_IE | ADC_CTL_END);
    ROM_ADCSequenceEnable(ADC0_BASE, ADC_SEQUENCE_3);
    ROM_ADCIntClear(ADC0_BASE, ADC_SEQUENCE_3);
#endif // not CYGWIN    
}

void io::setOutput(unsigned int out) {
    uint8_t pinTranslate = 0;
    if (out & PIN_0) {
        pinTranslate |= PIN_OUT_0;
    }
    if (out & PIN_1) {
        pinTranslate |= PIN_OUT_1;
    }
    if (out & PIN_2) {
        pinTranslate |= PIN_OUT_2;
    }
    if (out & PIN_3) {
        pinTranslate |= PIN_OUT_3;
    }
    if (out & PIN_4) {
        pinTranslate |= PIN_OUT_4;
    }
#if !CYGWIN    
    GPIOPinWrite(GPIO_OUT_PORT, PIN_OUT_ALL, pinTranslate);
#endif // not CYGWIN    
}

unsigned int io::getInput(void) const {
    unsigned int pinTranslate = 0;
#if CYGWIN
    uint32_t in = 0;
#else // not CYGWIN    
    uint32_t in = GPIOPinRead(GPIO_IN_PORT, PIN_IN_ALL);
#endif // CYGWIN    
    if (in & PIN_IN_0) {
        pinTranslate |= PIN_0;
    }
    if (in & PIN_IN_1) {
        pinTranslate |= PIN_1;
    }
    if (in & PIN_IN_2) {
        pinTranslate |= PIN_2;
    }
    if (in & PIN_IN_3) {
        pinTranslate |= PIN_3;
    }
    if (in & PIN_IN_4) {
        pinTranslate |= PIN_4;
    }
    return pinTranslate;
}

unsigned int io::getADC(unsigned int adcIndex) {
#if CYGWIN
#else // not CYGWIN
    ROM_ADCProcessorTrigger(ADC0_BASE, ADC_SEQUENCE_0);
    while(!ROM_ADCIntStatus(ADC0_BASE, ADC_SEQUENCE_0, false)) {}   // TODO: Bail out if nothing in several seconds
    ROM_ADCIntClear(ADC0_BASE, ADC_SEQUENCE_0);
    ROM_ADCSequenceDataGet(ADC0_BASE, ADC_SEQUENCE_0, _adcValue);
#endif // CYGWIN
    if (adcIndex >= ADC_COUNT) {
        return 0;
    }
    return _adcValue[adcIndex];
}

int io::getTemperature(void) {
    // From 13.3.6 pg. 780 of reference manual:
    // Vtsens = 2.7 - ((Temp + 55) / 75);
    // Temp = -(((Vtsens - 2.7) * 75) + 55);
    // From pg. 781: Temp = 147.5 - ((75 * (Vrefp - Vrefn) * ADC) / 4096)
    //  Where Vrefp = VDDA = 3.3v and Vrefn = GND
    //  Temp = 147.5 - (75 * 3.3 * ADC) / 4096
    //       = 147.5 - (247.5 * ADC) / 4096
#if CYGWIN
    return 21;  // room temperature
#else // not CYGWIN
    ROM_ADCProcessorTrigger(ADC0_BASE, ADC_SEQUENCE_3);
    while(!ROM_ADCIntStatus(ADC0_BASE, ADC_SEQUENCE_3, false)) {}   // TODO: Bail out if nothing in several seconds
    ROM_ADCIntClear(ADC0_BASE, ADC_SEQUENCE_3);
    ROM_ADCSequenceDataGet(ADC0_BASE, ADC_SEQUENCE_3, _temperatureValue);
    //return (int)(-_temperatureValue[0] * 75 + 147.5);    // TODO: Check this
    return (int)(147.5 - (247.5 * _temperatureValue[0]) / 4096);
#endif // CYGWIN
}

void io::bootloader(void) {
#if !CYGWIN
    #define SYSTICKS_PER_SECOND 100
    uint32_t ui32SysClock = ROM_SysCtlClockGet();
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / SYSTICKS_PER_SECOND);
    ROM_SysTickIntEnable();
    ROM_SysTickEnable();

    //USBDCDTerm(0); // Doesn't make any difference

    // Disable all interrupts
    ROM_IntMasterDisable();
    ROM_SysTickIntDisable();
    ROM_SysTickDisable();
    HWREG(NVIC_DIS0) = 0xffffffff;
    HWREG(NVIC_DIS1) = 0xffffffff;

    // 1. Enable USB PLL
    // 2. Enable USB controller
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_USB0);
    ROM_SysCtlPeripheralReset(SYSCTL_PERIPH_USB0);

    //ROM_SysCtlUSBPLLEnable();

    ROM_SysCtlUSBPLLEnable();

    // 3. Enable USB D+ D- pins

    // 4. Activate USB DFU
    ROM_SysCtlDelay(ui32SysClock / 3);
    ROM_IntMasterEnable(); // Re-enable interrupts at NVIC level
    ROM_UpdateUSB(0);
    // 5. Should never get here since update is in progress
#endif // not CYGWIN    
}