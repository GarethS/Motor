/*
	Copyright (c) Gareth Scott 2011, 2012, 2013

	stepper.h 

*/

#ifndef _STEPPER_H_
#define _STEPPER_H_

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
#else // not PART_TM4C1233D5PM
#include "gpio.h"
#include "timer.h"
#endif // PART_TM4C1233D5PM
#endif // CYGWIN

#include "accel.h"

#if CYGWIN
#include "log.h"
#include <sstream>

using namespace std;
#endif /* CYGWIN */

#define MAX_VIRTUAL_MOTOR_STEPS (10000)
#define MICROSTEPS_1_STD        (1.8)       // degrees per step for 1 microsteps, ie no microstepping -> 0.556 ustep/deg
#define MICROSTEPS_2_STD        (0.9)       // degrees per step for 2 microsteps                      -> 1.111 ustep/deg
#define MICROSTEPS_4_STD        (0.45)      // degrees per step for 4 microsteps                      -> 2.222 ustep/deg
#define MICROSTEPS_8_STD        (0.225)     // degrees per step for 8 microsteps                      -> 4.444 ustep/deg
#define MICROSTEPS_16_STD       (0.1125)    // degrees per step for 16 microsteps
#define MICROSTEPS_32_STD       (0.05625)   // degrees per step for 32 microsteps

enum microsteps {MICROSTEPS_1, MICROSTEPS_2, MICROSTEPS_4, MICROSTEPS_8, MICROSTEPS_16, MICROSTEPS_32};

#ifdef PART_TM4C1233D5PM
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

enum superstate {IDLE, MOVE_FULL, MOVE_TRUNCATED, VELOCITY_MOVE, SUPER_STATE_UNKNOWN};
	
enum substate {MOVE_START, MOVE_ACCELERATE, MOVE_CONSTANT_VELOCITY, MOVE_DECELERATE,
                VELOCITY_MOVE_ACCELERATE, VELOCITY_MOVE_DECELERATE, VELOCITY_MOVE_CONSTANT_VELOCITY,
                SUB_STATE_UNKNOWN};
// return values
enum {SUCCESS, ILLEGAL_MOVE, ILLEGAL_VELOCITY};                            
            
class stepper
#if CYGWIN
				: public logc
#endif /* CYGWIN */
				{
public:
    stepper();
    ~stepper() {}

	accel a;    // Handle math in acceleration class
	
    void step(void) {
        GPIOPinWrite(GPIO_STEP_PORT, PIN_STEP, PIN_STEP);
        if (_directionPositive) {
            ++_positionCurrent;
        } else {
            --_positionCurrent;
        }
        GPIOPinWrite(GPIO_STEP_PORT, PIN_STEP, ~PIN_STEP);
    }
      
    void directionPositive(bool positive = true) {
        _directionPositive = positive;
        if (_directionPositive) {
            GPIOPinWrite(GPIO_DIR_PORT, PIN_DIR, (unsigned char)~PIN_DIR);
        } else {
            GPIOPinWrite(GPIO_DIR_PORT, PIN_DIR, PIN_DIR);
        }
    }
    //bool directionPositive(void) {return _directionPositive;}
      
	// Only allow one of these, moveAbsolute() or velocity(), to be active at any one time.  
    void moveAbsolute(const int positionNew);
    void moveRelative(const int positionRelative) {moveAbsolute(_positionCurrent + positionRelative);}
    
    void moveAbsoluteDegreex10k(int positionNewDegreex10k) {moveAbsolute(positionNewDegreex10k / (int)degreesPerMicrostepx10k());}
    void moveRelativeDegreex10k(int positionRelativeDegreex10k) {moveRelative(positionRelativeDegreex10k / (int)degreesPerMicrostepx10k());}
    // Used to modify moveAbsolute() that is currently running. Useful to extend or shorten
    //  the move as long as it's still in the same direction and doesn't interfere with
    //  a deceleration.
    void moveAbsoluteModify(const int positionNew);
    void moveRelativeModify(const int positionRelative) {moveAbsoluteModify(_positionCurrent + positionRelative);}
    void controlledStopNow(void);
	int velocity(const int f);
    void isr(void);
    void disable(void) {GPIOPinWrite(GPIO_DIR_PORT, PIN_ENABLE, PIN_ENABLE);}
    void enable(void) {GPIOPinWrite(GPIO_DIR_PORT, PIN_ENABLE | PIN_SLEEP, PIN_SLEEP);}
    int positionSteps(void) const {return _positionCurrent;} // Need to return both degrees and scaled position.
    void positionSteps(const int p) {_positionCurrent = p;}   // set
    unsigned int positionDegreesx10k(void) const {return positionSteps() * (int)degreesPerMicrostepx10k();} // get
    void positionDegreesx10k(const unsigned int d) {_positionCurrent = d / (int)degreesPerMicrostepx10k();} // set
    superstate state(void) const {return _superState;}
    void accelerationTimeMicrosecs(const unsigned int us) {a.accelMicroSec(us);}   // Set acceleration time
    void frequency(const unsigned int flow, const unsigned int fhi) {a.frequency(flow, fhi);}  // Set high/low step frequency
    // Use RPMx10k when you want finer precision over speed than RPM can give you. e.g. RPM = 23.456 -> RPMx10k = 234560
    void RPMx10k(const unsigned int RPMx10kmin = 10000, const unsigned int RPMx10kmax = 1000000) {a.RPMx10k(RPMx10kmin, RPMx10kmax);}
    void RPM(const unsigned int RPMmin = 1, const unsigned int RPMmax = 100) {RPMx10k(RPMmin * 10000, RPMmax * 10000);}
    void microstepSet(const microsteps ms);
    void degreesPerMicrostepx10k(const unsigned int dpus) {a.degreesPerMicrostepx10k(dpus);}
    unsigned int degreesPerMicrostepx10k(void) const  {return a.degreesPerMicrostepx10k();}
	
#if CYGWIN    
	void test(void);
    void testMoveAbsolute(int positionNew);
    void testMoveAbsoluteDegree(int positionNewDegree);
    void testVelocityMove(int v);
    // Used to run a virtual motor and get the velocity profile to plot in a spreadsheet.
    void runVirtualMotor(unsigned int maxSteps = MAX_VIRTUAL_MOTOR_STEPS) {
        for (unsigned int i = 0; i < maxSteps && _superState != IDLE; ++i) {
            isr();
        }
    }
#endif /* CYGWIN */    
    
private:
    void _init(void);
	void _timerStart(bool start = true);
	void _timer(const unsigned long newClockPeriod) {
		_timerPeriod = newClockPeriod;
		TimerLoadSet(TIMER0_BASE, TIMER_A, _timerPeriod);
	}
	unsigned long _timer(void) const {return _timerPeriod;}
	void _updateConstantVelocityStart(void);
    
    // These tests are not stand-alone. You have to call _isAccelerating() before _isConstantVelocity() which is called before _isStartOfDecelerating().
    // See example of usage in stepper::isr()
    bool _isAccelerating(void) const {if ((_directionPositive && _positionCurrent < _positionConstantVelocityStart) ||
                                         (!_directionPositive && _positionCurrent > _positionConstantVelocityStart)) {return true;} return false;}
    bool _isConstantVelocity(void) const {if ((_directionPositive && _positionCurrent < _positionConstantVelocityEnd) ||
                                             (!_directionPositive && _positionCurrent > _positionConstantVelocityEnd)) {return true;} return false;}
    bool _isStartOfDeceleration(void) const {if (_positionCurrent == _positionConstantVelocityEnd) {return true;} return false;}
    bool _isDecelerating(void) const {if ((_directionPositive && _positionCurrent < _positionTarget) ||
                                         (!_directionPositive && _positionCurrent > _positionTarget)) {return true;} return false;}
    bool _isConstantVelocityStart(void) const {if (_positionCurrent == _positionConstantVelocityStart) {return true;} return false;}
	
	// These 4 points represent a typical movement profile.
	//  Acceleration is from _positionCurrent to _positionConstantVelocityStart
	//  Constant velocity is from _positionConstantVelocityStart to _positionConstantVelocityEnd
	//  Deceleration is from _positionConstantVelocityEnd to _positionTarget
	// If, instead, a velocity() command is being issued, only 2 of the points are used.
	//  Increasing velocity is from _positionCurrent to _positionConstantVelocityStart
	//  Decreasing velocity is from _positionConstantVelocityEnd to _positionTarget
	// To change direction, 2 velocity, or moveAbsolute, commands should be used. One to bring
	//  the motor to 0 velocity and the next to move in the opposite direction.
    int _positionCurrent;  // in microsteps
	int _positionConstantVelocityStart;
	int _positionConstantVelocityEnd;
	int _positionTarget;
	
    bool _directionPositive;            // false -> direction is negative
	bool _timerRunning;
	unsigned long _timerPeriod;
    accel _accelPrevious;               // used after a truncated move to revert back to original acceleration
	superstate _superState;
	substate _subState;
#if REGRESS_2 && !CYGWIN
	unsigned int _superStateLast;
	unsigned int _subStateLast;
#endif /* REGRESS_2 */    
};

#endif /* _STEPPER_H_ */