/*
	Copyright (c) Gareth Scott 2011

	stepper.h 

*/

#ifndef _STEPPER_H_
#define _STEPPER_H_

#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_types.h"
#include "gpio.h"
#include "lmi_timer.h"

#include "accel.h"

#if CYGWIN
//#include <iostream>
#include "log.h"
#include <sstream>

using namespace std;
#endif /* CYGWIN */

#define OPTIMIZE_CURVE_CALC 1

#define PIN_ENABLE  (GPIO_PIN_4)
#define PIN_SLEEP   (GPIO_PIN_5)
#define PIN_STEP    (GPIO_PIN_6)
#define PIN_DIR     (GPIO_PIN_7)
#define PIN_ALL     (PIN_ENABLE | PIN_SLEEP | PIN_STEP | PIN_DIR)

class stepper
#if CYGWIN
				: public logc
#endif /* CYGWIN */
				{
public:
    stepper();
    ~stepper() {}

	accel a;
	
    void step(void) {
        GPIOPinWrite(GPIO_PORTA_BASE, PIN_STEP, PIN_STEP);
        if (_directionPositive) {
            ++_positionCurrent;
        } else {
            --_positionCurrent;
        }
        GPIOPinWrite(GPIO_PORTA_BASE, PIN_STEP, ~PIN_STEP);
    }
      
    void directionPositive(bool positive = true) {
        _directionPositive = positive;
        if (_directionPositive) {
            GPIOPinWrite(GPIO_PORTA_BASE, PIN_DIR, (unsigned char)~PIN_DIR);
        } else {
            GPIOPinWrite(GPIO_PORTA_BASE, PIN_DIR, PIN_DIR);
        }
    }
      
	// Only allow one of these, moveAbsolute() or velocity(), to be active at any one time.  
    void moveAbsolute(int positionNew);
	void velocity(const unsigned int f);
    void isr(void);
    //int acceleration(void) {/* TODO */}
    //int velocity(void) {/* TODO */}
    //int position(void) {/* TODO */} // Need to return both degrees and scaled position.
	
	void test(void);
    
private:
	// _superState
    enum {IDLE, MOVE_FULL, MOVE_TRUNCATED, VELOCITY_MOVE};
	
	// _subState
	enum {MOVE_START, MOVE_ACCELERATE, MOVE_CONSTANT_VELOCITY, MOVE_DECELERATE,
			VELOCITY_MOVE_ACCELERATE, VELOCITY_MOVE_DECELERATE, VELOCITY_MOVE_CONSTANT_VELOCITY};

    void _init(void);
    
private:
	void _timerStart(bool start = true);
	void _timerSet(unsigned long newClockPeriod) {
		_timerPeriod = newClockPeriod;
		TimerLoadSet(TIMER0_BASE, TIMER_A, newClockPeriod);
	}
	
	// These 4 points represent a typical movement profile.
	//  Acceleration is from _positionCurrent to _positionConstantVelocityStart
	//  Constant velocity is from _positionConstantVelocityStart to _positionConstantVelocityEnd
	//  Deceleration is from _positionConstantVelocityEnd to _positionTarget
	// If, instead, a velocity() command is being issued, only 2 of the points are used.
	//  Increasing velocity is from _positionCurrent to _positionConstantVelocityStart
	//  Decreasing velocity is from _positionConstantVelocityEnd to _positionTarget
	// To change direction, 2 velocity, or moveAbsolute, commands should be used. One to bring
	//  the motor to 0 velocity and the next to move in the opposite direction.
    int _positionCurrent;  // in steps
	int _positionConstantVelocityStart;
	int _positionConstantVelocityEnd;
	int _positionTarget;
	
    bool _directionPositive;    // false -> direction is negative
	bool _timerRunning;
	unsigned long _timerPeriod;
    //int _acceleration;          // 0 when constant velocity
    //bool _accelerating; 
    //int _velocity;              // step/s
	unsigned int _fminOld, _fmaxOld;	// save frequency min/max for acceleration. Used when acceleration is trucated.
	unsigned int _superState;
	unsigned int _subState;
};

#endif /* _STEPPER_H_ */