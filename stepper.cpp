/*
	Copyright (c) Gareth Scott 2011

	stepper.cpp 

*/

#include "stepper.h"
#include "lmi_timer.h"
#include <math.h>

stepper::stepper() :
#if CYGWIN 
					logc(std::string("STEPPER")),
#endif /* CYGWIN */					
					_positionCurrent(0), _directionPositive(true), _timerRunning(false), _superState(IDLE) {
}

void stepper_init(void) {
    GPIODirModeSet(GPIO_PORTA_BASE, PIN_ALL, GPIO_DIR_MODE_OUT);
    GPIOPadConfigSet(GPIO_PORTA_BASE, PIN_ALL, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);
}

// Set motor to run continuously at a given velocity. To stop, set velocity to 0.
//  This function cannot be used until moveAbsolute() has completed.
void stepper::velocity(const unsigned int f) {
	if (_superState == IDLE) {
		// Starting from f = 0.
		unsigned int accelStep = a.timeToSteps(a.time());
		if (_directionPositive) {
			_positionConstantVelocityStart = _positionCurrent + accelStep;;
		} else {
			_positionConstantVelocityStart = _positionCurrent - accelStep;;
		}
	} else {
		int fdiff = f - a.freqFromClockTicks(_timerPeriod);
		if (fdiff == 0) {
			// nothing to do
			return;
		} else if (fdiff > 0) {
			_subState = VELOCITY_MOVE_ACCELERATE;
			a.frequency(a.freqFromClockTicks(_timerPeriod), f);
		} else {
			// fdiff < 0
			_subState = VELOCITY_MOVE_DECELERATE;
			a.frequency(f, a.freqFromClockTicks(_timerPeriod));
		}
		unsigned int accelStep = a.timeToSteps(a.time());
		if (_directionPositive) {
			_positionConstantVelocityStart = _positionCurrent + accelStep;;
		} else {
			_positionConstantVelocityStart = _positionCurrent - accelStep;;
		}
	}
}

// Set motor to run continuously at a given velocity. To stop, set velocity to 0.
//  This function cannot be used until velocity() has brought motor to a halt.
void stepper::moveAbsolute(int positionNew) {
	if (_superState != IDLE) {
		// Still running.
		return;
	}
	unsigned int positionDelta;
	_positionTarget = positionNew;
	if (_positionTarget > _positionCurrent) {
		directionPositive();
		positionDelta = _positionTarget - _positionCurrent;
	} else {
		directionPositive(false);
		positionDelta = _positionCurrent - _positionTarget;
	}
	// 0. Initialize timer
	// 1. Start timer
	// 2. Ramp up through acceleration curve
	// 3. Maintain max speed until time to decelerate
	// 4. Ramp down acceleration curve (run acceleration curve backwards)
	// 5. Stop timer.
	
	// 1. Set acceleration time (e.g. 0.5s)
	unsigned int accelStep = a.timeToSteps(a.time());
	if (positionDelta > 2 * accelStep) {
		// Enough room for full acceleration profile
		if (_directionPositive) {
			_positionConstantVelocityStart = _positionCurrent + accelStep;
			_positionConstantVelocityEnd = _positionTarget - accelStep;
		} else {
			_positionConstantVelocityStart = _positionCurrent - accelStep;
			_positionConstantVelocityEnd = _positionTarget + accelStep;
		}
		_superState = MOVE_FULL;
	} else {
		// Acceleration curve needs to be truncated. Not enough room to reach max speed.
		unsigned int tNew = a.stepsToTime(positionDelta / 2);
		// Set fMAX and then calculate time required for acceleration.
		unsigned int us = a.microSecToCurveIndex(tNew);
		_fminOld = a.fmin();
		_fmaxOld = a.fmax();
		unsigned int fmax = a.freqFromTime(us);
		a.frequency(_fminOld, fmax);	// Rebuilds acceleration tables. Put them back when this move is finished.
		// Set truncated acceleration time.
		a.primeTime(tNew);
		// Remember to rebuild accel tables when move is over. Set trigger to do this in isr().
		if (_directionPositive) {
			_positionConstantVelocityStart = _positionConstantVelocityEnd = _positionCurrent + positionDelta / 2;
		} else {
			_positionConstantVelocityStart = _positionConstantVelocityEnd = _positionCurrent - positionDelta / 2;
		}
		_superState = MOVE_TRUNCATED;
	}
	_subState = MOVE_START;
	_timerStart();
}

// Called every time timer times out.
void stepper::isr(void) {
	step();
	if (_superState == MOVE_FULL || _superState == MOVE_TRUNCATED) {
		if (_directionPositive) {
			if (_positionCurrent  < _positionConstantVelocityStart) {
				// accelerating
				_subState = MOVE_ACCELERATE;
				unsigned int newClockPeriod = a.updateClockPeriod();
				TimerLoadSet(TIMER0_BASE, TIMER_A, newClockPeriod);
			} else if (_positionCurrent > _positionConstantVelocityEnd) {
				// decelerating
				_subState = MOVE_DECELERATE;
				unsigned int newClockPeriod = a.updateClockPeriodReverse();
				TimerLoadSet(TIMER0_BASE, TIMER_A, newClockPeriod);
			} else if (_positionCurrent == _positionConstantVelocityEnd) {
				// Start of deceleration.
				_subState = MOVE_DECELERATE;
				a.primeTime(a.time());
			} else if (_positionCurrent >= _positionTarget) {
				_superState = IDLE;
				_timerStart(false);
			} else {
				_subState = MOVE_CONSTANT_VELOCITY;
				// constant velocity. Nothing to do.
			}
		} else {
			if (_positionCurrent  > _positionConstantVelocityStart) {
				// accelerating
				_subState = MOVE_ACCELERATE;
				unsigned int newClockPeriod = a.updateClockPeriod();
				TimerLoadSet(TIMER0_BASE, TIMER_A, newClockPeriod);
			} else if (_positionCurrent < _positionConstantVelocityEnd) {
				// decelerating
				_subState = MOVE_DECELERATE;
				unsigned int newClockPeriod = a.updateClockPeriodReverse();
				TimerLoadSet(TIMER0_BASE, TIMER_A, newClockPeriod);
			} else if (_positionCurrent == _positionConstantVelocityEnd) {
				// Start of deceleration.
				_subState = MOVE_DECELERATE;
				a.primeTime(a.time());
			} else if (_positionCurrent <= _positionTarget) {
				_timerStart(false);
			} else {
				_superState = IDLE;
				_subState = MOVE_CONSTANT_VELOCITY;
				// constant velocity. Nothing to do.
			}
		}
	} else if (_superState == VELOCITY_MOVE) {
	}
}

void stepper::_timerStart(bool start /* = true */) {
	if (start) {
		_timerRunning = true;
		TimerEnable(TIMER0_BASE, TIMER_A);
	} else {
		_timerRunning = false;
		TimerDisable(TIMER0_BASE, TIMER_A);
		if (_superState == MOVE_TRUNCATED) {
			// Reset acceleration tables
			a.frequency(_fminOld, _fmaxOld);
		}
		_superState = IDLE;
	}
}