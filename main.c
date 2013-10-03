//*****************************************************************************
//
// uart_echo.c - Example for reading data from and writing data to the UART in
//               an interrupt driven fashion.
//
// Copyright (c) 2008 Luminary Micro, Inc.  All rights reserved.
// 
// Software License Agreement
// 
// Luminary Micro, Inc. (LMI) is supplying this software for use solely and
// exclusively on LMI's microcontroller products.
// 
// The software is owned by LMI and/or its suppliers, and is protected under
// applicable copyright laws.  All rights are reserved.  You may not combine
// this software with "viral" open-source software in order to form a larger
// program.  Any use in violation of the foregoing restrictions may subject
// the user to criminal sanctions under applicable laws, as well as to civil
// liability for the breach of the terms and conditions of this license.
// 
// THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
// OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
// LMI SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
// CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 2617 of the Stellaris Peripheral Driver Library.
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_types.h"
#ifdef PART_TM4C1233D5PM
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/rom.h"
#include "driverlib/timer.h"

#include "usblib/usblib.h"      // provides definition of: tUSBBuffer
#else // not PART_TM4C1233D5PM
#include "debug.h"
#include "gpio.h"
#include "interrupt.h"
#include "sysctl.h"
#include "uart.h"
#include "rom.h"
//#include "grlib/grlib.h"
//#include "formike128x128x16.h"
#endif // PART_TM4C1233D5PM

//#include "inc/lm3s3748.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include <assert.h>

void vApplicationStackOverflowHook( xTaskHandle *pxTask, signed portCHAR *pcTaskName );
void vApplicationTickHook( void );
void vSetupHighFrequencyTimer( void );
extern void bufferInput(unsigned char c);
extern void stepper_init(void);

int mainA(void);

void localAssert(void) {
//void localAssert(void* FILE, int LINE) {
    assert(false);
}


/* The task that is created three times. */
//#define ledSTACK_SIZE		(configMINIMAL_STACK_SIZE)
#define ledSTACK_SIZE		(400)
#define mainLED_TASK_PRIORITY           ( tskIDLE_PRIORITY + 1 )

void vStartLEDOnTasks( unsigned portBASE_TYPE uxPriority );
static void vLEDOnTask(void* pvParameters );

void vStartIdleTask( unsigned portBASE_TYPE uxPriority );
static void vIdleTask(void* pvParameters );

void vStartUARTTasks( unsigned portBASE_TYPE uxPriority );
static void vUARTTask(void* pvParameters );

#ifdef PART_TM4C1233D5PM
extern const tUSBBuffer g_sTxBuffer;
extern const tUSBBuffer g_sRxBuffer;
// used for testing
#define PIN_ENABLE  (GPIO_PIN_4)    // PORTC
#define PIN_SLEEP   (GPIO_PIN_7)    // PORTC
#define PIN_STEP    (GPIO_PIN_6)    // PORTD
#define PIN_DIR     (GPIO_PIN_5)    // PORTC
#define GPIO_STEP_PORT  (GPIO_PORTD_BASE)
#define GPIO_DIR_PORT   (GPIO_PORTC_BASE)
#else // not PART_TM4C1233D5PM
#define PIN_ENABLE  (GPIO_PIN_4)
#define PIN_SLEEP   (GPIO_PIN_5)
#define PIN_STEP    (GPIO_PIN_6)
#define PIN_DIR     (GPIO_PIN_7)
#define PIN_ALL     (PIN_ENABLE | PIN_SLEEP | PIN_STEP | PIN_DIR)
#define GPIO_STEP_PORT  (GPIO_PORTA_BASE)
#define GPIO_DIR_PORT   (GPIO_PORTA_BASE)
#endif // PART_TM4C1233D5PM

//*****************************************************************************
//
//! \addtogroup ek_lm3s3748_list
//! <h1>UART (uart_echo)</h1>
//!
//! This example application utilizes the UART to echo text.  The first UART
//! (connected to the FTDI virtual serial port on the evaluation board) will be
//! configured in 115,200 baud, 8-n-1 mode.  All characters received on the
//! UART are transmitted back to the UART.
//
//*****************************************************************************

//*****************************************************************************
//
// Graphics context used to show text on the CSTN display.
//
//*****************************************************************************
//tContext g_sContext;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
}
#endif

void UARTSend(const unsigned char *pucBuffer, unsigned long ulCount);
void flashLED(void);


//*****************************************************************************
//
// The UART interrupt handler.
//
//*****************************************************************************
void
UARTIntHandler(void)
{
    unsigned long ulStatus;

    //
    // Get the interrrupt status.
    //
    ulStatus = ROM_UARTIntStatus(UART0_BASE, true);

    //
    // Clear the asserted interrupts.
    //
    ROM_UARTIntClear(UART0_BASE, ulStatus);

    //UARTSend("x", 1);
    //flashLED();
    
    //
    // Loop while there are characters in the receive FIFO.
    //
    while(ROM_UARTCharsAvail(UART0_BASE))
    {
        //
        // Read the next character from the UART and write it back to the UART.
        //
        //ROM_UARTCharPutNonBlocking(UART0_BASE, ROM_UARTCharGetNonBlocking(UART0_BASE));
        bufferInput(ROM_UARTCharGetNonBlocking(UART0_BASE));
    }
}

//*****************************************************************************
//
// Send a string to the UART.
//
//*****************************************************************************
void
UARTSend(const unsigned char *pucBuffer, unsigned long ulCount)
{
#ifdef PART_TM4C1233D5PM
    USBBufferWrite((tUSBBuffer *)&g_sTxBuffer, (uint8_t *)pucBuffer, ulCount);
#else // not PART_TM4C1233D5PM    
    //
    // Loop while there are more characters to send.
    //
    while(ulCount--)
    {
        //
        // Write the next character to the UART.
        //
        ROM_UARTCharPutNonBlocking(UART0_BASE, *pucBuffer++);
    }
#endif // PART_TM4C1233D5PM    
}

void delay(void) {
#if 0
    //static const portTickType xDelay = 100 / portTICK_RATE_MS;
    static const portTickType xDelay = 1;
    static portTickType xTickCount;
    xTickCount = xTaskGetTickCount();

    vTaskDelay( 3 );
    xTickCount = xTaskGetTickCount();
#else
  for (volatile unsigned int ulLoop = 0; ulLoop < 5000; ulLoop++) {
  }
#endif
}

void LEDOn(void) {
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0);
    //GPIO_PORTF_DATA_R |= 0x01;  // LED on
}

void LEDOff(void) {
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, ~GPIO_PIN_0);
    //GPIO_PORTF_DATA_R &= ~(0x01); // LED off
}

void enable(void) {
    GPIOPinWrite(GPIO_DIR_PORT, PIN_ENABLE | PIN_SLEEP, PIN_SLEEP);
    //GPIOPinWrite(GPIO_PORTA_BASE, PIN_ENABLE , PIN_ENABLE );
}

void motorStep(void) {
    GPIOPinWrite(GPIO_STEP_PORT, PIN_STEP, PIN_STEP);
    delay();
    GPIOPinWrite(GPIO_STEP_PORT, PIN_STEP, ~PIN_STEP);
    delay();
}

void flashLED(void) {
#if 0
    LEDOn();
    delay();
    LEDOff();
    delay();
#endif    
}

#ifdef PART_TM4C1233D5PM
#define timerHIGHEST_PRIORITY			( 0 )
#define timerINTERRUPT_FREQUENCY		( 200UL )
void vSetupHighFrequencyTimer( void )
{
	/* Timer zero is used to generate the interrupts, and timer 1 is used
	to measure the jitter. */
	SysCtlPeripheralEnable( SYSCTL_PERIPH_TIMER0 );
    SysCtlPeripheralEnable( SYSCTL_PERIPH_TIMER1 );
    TimerConfigure( TIMER0_BASE, TIMER_CFG_PERIODIC );
    //TimerConfigure( TIMER1_BASE, TIMER_CFG_PERIODIC );
	
	/* Set the timer interrupt to be above the kernel - highest. */
	IntPrioritySet( INT_TIMER0A, timerHIGHEST_PRIORITY );

	/* Just used to measure time. */
    //TimerLoadSet(TIMER1_BASE, TIMER_A, timerMAX_32BIT_VALUE );
	
	/* Ensure interrupts do not start until the scheduler is running. */
	portDISABLE_INTERRUPTS();
	
	/* The rate at which the timer will interrupt. */
	unsigned long ulTimerCount = configCPU_CLOCK_HZ / timerINTERRUPT_FREQUENCY;	
    TimerLoadSet( TIMER0_BASE, TIMER_A, ulTimerCount );
    IntEnable( INT_TIMER0A );
    TimerIntEnable( TIMER0_BASE, TIMER_TIMA_TIMEOUT );

	/* Enable both timers. */	
    TimerEnable( TIMER0_BASE, TIMER_A );
    //TimerEnable( TIMER1_BASE, TIMER_A );
}
#endif // PART_TM4C1233D5PM

//*****************************************************************************
//
// This example demonstrates how to send a string of data to the UART.
//
//*****************************************************************************
int
#ifdef PART_TM4C1233D5PM
mainA(void) {
#else // not PART_TM4C1233D5PM
main(void) {
    //tRectangle sRect;

    //
    // Set the clocking to run directly from the crystal.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);

    // A lot of people use this:
    //SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);
    
    //
    // Enable the peripherals used by this example.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Enable processor interrupts.
    //
    IntMasterEnable();

    //
    // Set GPIO A0 and A1 as UART pins.
    //
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    
    // Following 2 lines replaced by stepper_init() below
    //GPIODirModeSet(GPIO_PORTA_BASE, PIN_ALL, GPIO_DIR_MODE_OUT);
    //GPIOPadConfigSet(GPIO_PORTA_BASE, PIN_ALL, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);

    //
    // Configure the UART for 115,200, 8-N-1 operation.
    //
    ROM_UARTConfigSetExpClk(UART0_BASE, ROM_SysCtlClockGet(), /*2400*/ 115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    //
    // Enable the UART interrupt.
    //
    ROM_IntEnable(INT_UART0);
    ROM_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

    
#if 1
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_DIR_MODE_OUT);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);
#else
    SYSCTL_RCGC2_R = SYSCTL_RCGC2_GPIOF;  // Enable GPIO port
    volatile unsigned long ulLoop;
    ulLoop = SYSCTL_RCGC2_R;  // Dummy read to give peripheral time to turn on
    GPIO_PORTF_DIR_R = 0x01;    // Sets io direction
    GPIO_PORTF_DEN_R = 0x01;    // Digital enable
#endif
    
    //
    // Prompt for text to be entered.
    //
    UARTSend((unsigned char *)"Start", 5);
    flashLED();
#endif // PART_TM4C1233D5PM
    stepper_init();

#ifdef PART_TM4C1233D5PM    
    vStartIdleTask(tskIDLE_PRIORITY);
#endif // PART_TM4C1233D5PM    
    vStartLEDOnTasks(mainLED_TASK_PRIORITY);
    //vStartUARTTasks(mainLED_TASK_PRIORITY);
    vSetupHighFrequencyTimer();
    vTaskStartScheduler();
    // Normally will never reach here.
    
    // Loop forever echoing data through the UART.
    while(1)
    {
#if 0
      GPIO_PORTF_DATA_R |= 0x01;  // LED on
      delay();
      GPIO_PORTF_DATA_R &= ~(0x01); // LED off
#endif
      //flashLED();
      for (int x = 0; x < 10; ++x) {
        delay();      
      }
      //UARTSend((unsigned char *)"Enter text: ", 12);
      UARTSend((unsigned char *)"<H>", 3);
      
      if (UARTCharsAvail(UART0_BASE)) {
        flashLED();
      }
      
    }
}

void vStartIdleTask( unsigned portBASE_TYPE uxPriority )
{
    xTaskCreate( vIdleTask, (signed char *)"Idle", 32 /*ledSTACK_SIZE*/, NULL, uxPriority, ( xTaskHandle * ) NULL );
}

static void vIdleTask(void* pvParameters) {
#define IDLE_MS (500)    
    //static unsigned portBASE_TYPE uxHighWaterMark;

    for (;;) {
        //uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, GPIO_PIN_0);
        vTaskDelay(IDLE_MS / portTICK_RATE_MS);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, 0);
        vTaskDelay(IDLE_MS / portTICK_RATE_MS);
    }
}

void vStartLEDOnTasks( unsigned portBASE_TYPE uxPriority )
{
    //signed portBASE_TYPE xLEDTask;

    xTaskCreate( vLEDOnTask, ( signed char * ) "LEDx", 2600 /*ledSTACK_SIZE*/, NULL, uxPriority, ( xTaskHandle * ) NULL );
}

void interpretRun(void);

static void vLEDOnTask(void* pvParameters )
{
#if 0
    // Used to initially test 2 tasks running simultaneously. The idle task blinked the other LED.
    for (;;) {
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_1, GPIO_PIN_1);
        vTaskDelay(505 / portTICK_RATE_MS);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_1, 0);
        vTaskDelay(505 / portTICK_RATE_MS);
    }
#endif    
//portTickType xFlashRate, xLastFlashTime;
//unsigned portBASE_TYPE uxLED;
    enable(); // Turn this on for the plain stepper demo
    interpretRun();
    //mainA();
    for (;;) {
       	//portENTER_CRITICAL();
#if 0
        //vTaskDelay(1 / portTICK_RATE_MS);
        //UARTSend((unsigned char *)"<L>", 3);
        LEDOn();
        //vTaskDelay(1 / portTICK_RATE_MS);
        LEDOff();
#else        
        vTaskDelay(1);
        flashLED();
#endif        
        motorStep();
        //LEDOn();
       	//portEXIT_CRITICAL();
    }
}

void vStartUARTTasks( unsigned portBASE_TYPE uxPriority )
{
    //signed portBASE_TYPE xLEDTask;

    /* Spawn the task. */
    xTaskCreate( vUARTTask, ( signed char * ) "UARTx", ledSTACK_SIZE, NULL, uxPriority, ( xTaskHandle * ) NULL );
}

static portTASK_FUNCTION( vUARTTask, pvParameters )
{
//portTickType xFlashRate, xLastFlashTime;
//unsigned portBASE_TYPE uxLED;
    for (;;) {
       	//portENTER_CRITICAL();
#if 0
        vTaskDelay( 300 );
#else
        for (int x = 0; x < 10; ++x) {
            delay();      
        }
#endif
        //UARTSend((unsigned char *)"Enter text: ", 12);
        //UARTSend((unsigned char *)"<B>", 3);
       	//portEXIT_CRITICAL();
    }
}

void vApplicationTickHook( void )
{
}

void vApplicationStackOverflowHook( xTaskHandle *pxTask, signed portCHAR *pcTaskName )
{
	( void ) pxTask;
	( void ) pcTaskName;

	for( ;; );
}
