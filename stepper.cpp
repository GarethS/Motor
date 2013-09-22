/*
	Copyright (c) Gareth Scott 2011, 2012, 2013

	stepper.cpp 

*/

#include "stepper.h"
#include <math.h>
#include <assert.h>

#if !CYGWIN
#include <stdio.h>
#include <string.h>

extern "C" void UARTSend(const unsigned char *pucBuffer, unsigned long ulCount);
#ifdef PART_TM4C1233D5PM
stepper* sp = NULL;

extern "C" void stepperISR(void) {
    if (sp != NULL) {
        sp->isr();
    }
}
#endif // PART_TM4C1233D5PM
#endif /* not CYGWIN */

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

#if CYGWIN
void stepper::testMoveAbsolute(int positionNew) {
	oss() << "moveAbsolute START" << endl;
    //a.accelTime(500000);
	moveAbsolute(positionNew);
	oss() << "_positionCurrent=" << _positionCurrent <<
			" _positionConstantVelocityStart=" << _positionConstantVelocityStart <<
			" _positionConstantVelocityEnd=" << _positionConstantVelocityEnd <<
			" _positionTarget=" << _positionTarget << endl;
	oss() << "moveAbsolute END";
	dump();
    runVirtualMotor();
}

void stepper::testMoveAbsoluteDegree(int positionNewDegree) {
    oss() << endl << "testMoveAbsoluteDegree START" << endl;
    testMoveAbsolute((int)(positionNewDegree / degreesPerMicrostep()));
}

void stepper::testVelocityMove(int v) {
	oss() << endl << "velocity START" << endl;
	velocity(v);
	oss() << "_positionCurrent=" << _positionCurrent <<
			" _positionConstantVelocityStart=" << _positionConstantVelocityStart << endl;
	oss() << "velocity END";
	dump();
    runVirtualMotor(2000);
    _superState = IDLE; // So next test can run without thinking it's still in VELOCITY_MOVE
}

void stepper::test(void) {
    testMoveAbsolute(2000);
    testMoveAbsolute(0);

    testVelocityMove(800);

    positionSteps(0);
    degreesPerMicrostep(MICROSTEPS_8_STD);
    testMoveAbsoluteDegree(360);
}
#endif /* CYGWIN */

// Set motor to run continuously at a given velocity. To stop, set velocity to 0.
//  This function cannot be used until moveAbsolute() has completed.
int stepper::velocity(const int f) {
	if (_superState == IDLE) {
		// Starting from f = 0.
		if (f == 0) {
			return SUCCESS;	// nothing to do
        } else if (f > 0) {
            directionPositive();
            a.frequency(a.fmin(), f);
        } else {
            // f < 0
            directionPositive(false);
            a.frequency(a.fmin(), -f);
        }
		_updateConstantVelocityStart();
        _superState = VELOCITY_MOVE;
		a.initAccelTime(a.accelTime());
	} else if (_superState == VELOCITY_MOVE_ACCELERATE || _superState == VELOCITY_MOVE_DECELERATE) {
		// Can't do it right now. Try again when we're done accelerating.
	} else {
        // motor already running at a constant velocity
        assert(_superState == VELOCITY_MOVE);
        if ((f > 0 && !_directionPositive) || (f < 0 && _directionPositive)) {
            // Trying to change direction. Can't do this. Instead, bring motor to a stop and then issue another velocity() command to
            //  reverse direction.
            return ILLEGAL_VELOCITY;
        }
		int fdiff = f - a.freqFromClockTicks(_timerPeriod);
		if (fdiff == 0) {
			// nothing to do
			assert(_subState == VELOCITY_MOVE_CONSTANT_VELOCITY);
			return SUCCESS;
		} else if (fdiff > 0) {
			_subState = VELOCITY_MOVE_ACCELERATE;
			a.frequency(a.freqFromClockTicks(_timerPeriod), f);
		} else {
			// fdiff < 0
			_subState = VELOCITY_MOVE_DECELERATE;
			a.frequency(f, a.freqFromClockTicks(_timerPeriod));
		}
		_updateConstantVelocityStart();
	}
	_timerStart();
    return SUCCESS;
}

void stepper::_updateConstantVelocityStart(void) {
	unsigned int accelStep = a.timeToSteps(a.accelTime());
	if (_directionPositive) {
		_positionConstantVelocityStart = _positionCurrent + accelStep;;
	} else {
		_positionConstantVelocityStart = _positionCurrent - accelStep;;
	}
	_subState = VELOCITY_MOVE_ACCELERATE;
}

void stepper::moveAbsolute(int positionNew) {
#if CYGWIN
    oss() << "moveAbsolute:" << positionNew << endl;
    dump();
#endif // CYGWIN
	if (_superState != IDLE) {
		// Can't do it right now. Try again when we're done accelerating.
		return;
	}
	unsigned int positionDelta;
	_positionTarget = positionNew;
	if (_positionTarget > _positionCurrent) {
		directionPositive();
		positionDelta = _positionTarget - _positionCurrent;
	} else if (_positionTarget < _positionCurrent) {
		directionPositive(false);
		positionDelta = _positionCurrent - _positionTarget;
	} else {
        // _positionTarget == _positionCurrent
        // Nothing to do.
        return;
    }
	// 0. Initialize timer
	// 1. Start timer
	// 2. Ramp up through acceleration curve
	// 3. Maintain max speed until time to decelerate
	// 4. Ramp down acceleration curve (run acceleration curve backwards)
	// 5. Stop timer.
	
	// 1. Set acceleration time (e.g. 0.5s)
	unsigned int accelStep = a.timeToSteps(a.accelTime());
#if REGRESS_1
	oss() << "moveAbsolute() accelStep=" << accelStep << " positionDelta=" << positionDelta << endl;
#endif /* REGRESS_1 */
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
		a.initAccelTime(a.accelTime());
	} else {
		// Acceleration curve needs to be truncated. Not enough steps to reach max speed.
#if REGRESS_1
        oss() << "moveAbsolute() MOVE_TRUNCATED steps=" << accelStep << " positionDelta=" << positionDelta << endl;
#endif /* REGRESS_1 */
#if REGRESS_2 && !CYGWIN
        char isrBuf[32];
        //sprintf(isrBuf, "tNewAccel=%d", tNewAccel);
        sprintf(isrBuf, " pt=%d", _positionTarget);
        //sprintf(isrBuf, "fo=%d max=%d", _fmaxOld, fmax);
        UARTSend((unsigned char *)isrBuf, strlen(isrBuf));
#endif /* REGRESS_2 and not CYGWIN */        
		unsigned int tNewAccel = a.stepsToTime(positionDelta / 2);
		// Set fMAX and then calculate time required for acceleration.
		unsigned int curveIndex = a.microSecToCurveIndex(tNewAccel);
        unsigned int ct = a.curveIndexToClockTicks(curveIndex);
        unsigned int us = a.clockTicksToMicroSec(ct);
		_fminOld = a.fmin();
		_fmaxOld = a.fmax();
		unsigned int fmax = a.freqFromTime(us);
#if REGRESS_2 && !CYGWIN
        //char isrBuf[32];
        //sprintf(isrBuf, "tNewAccel=%d", tNewAccel);
        //sprintf(isrBuf, "us=%d", us);
        sprintf(isrBuf, "ci=%d ct=%d", curveIndex, ct);
        //sprintf(isrBuf, "fo=%d fm=%d", _fmaxOld, fmax);
        UARTSend((unsigned char *)isrBuf, strlen(isrBuf));
#endif /* REGRESS_2 and not CYGWIN */        
		a.frequency(_fminOld, fmax);	// Rebuilds acceleration tables. Put them back when this move is finished.
		// Set truncated acceleration time.
		a.initAccelTime(tNewAccel);
		// Remember to rebuild accel tables when move is over. Set trigger to do this in isr().
		if (_directionPositive) {
			_positionConstantVelocityStart = _positionConstantVelocityEnd = _positionCurrent + positionDelta / 2;
		} else {
			_positionConstantVelocityStart = _positionConstantVelocityEnd = _positionCurrent - positionDelta / 2;
		}
#if REGRESS_1
        oss() << "moveAbsolute() MOVE_TRUNCATED _positionConstantVelocityStart=" << _positionConstantVelocityStart << " positionDelta=" << positionDelta << endl;
#endif /* REGRESS_1 */
		_superState = MOVE_TRUNCATED;
	}
	_subState = MOVE_START;
#if REGRESS_2 && !CYGWIN    
    _subStateLast = SUB_STATE_UNKNOWN;
    _superStateLast = SUPER_STATE_UNKNOWN;
#endif /* REGRESS_2 and not CYGWIN */    
	_timerStart();
}

void stepper::moveRelative(const int positionRelative) {
    moveAbsolute(_positionCurrent + positionRelative);
}

void stepper::moveRelativeModify(const int positionRelative) {
    moveAbsoluteModify(_positionCurrent + positionRelative);
}

// Start decellerating now to bring motor to a controlled stop
void stepper::controlledStopNow(void) {
    if (_superState == MOVE_FULL) {
        if (_subState == MOVE_CONSTANT_VELOCITY) {
            // move _positionTarget closer so we're just ready to enter the deceleration phase
            if (_directionPositive) {
                moveAbsoluteModify(_positionConstantVelocityEnd - _positionCurrent);
            } else {
                // direction is negative
                moveAbsoluteModify(_positionConstantVelocityEnd + _positionCurrent);
            }
        } else if (_subState == MOVE_ACCELERATE) {
            // TODO
        }
    }
}

void stepper::moveAbsoluteModify(int positionNew) {
    if (_superState == IDLE) {
        moveAbsolute(positionNew);
    } else if (_superState == MOVE_FULL && _subState != MOVE_DECELERATE) {
        // Already running a moveAbsolute(), see if it can be adjusted
        if (_directionPositive) {
            if (positionNew > _positionCurrent) {
                //int positionDelta = positionNew - _positionTarget;
                _positionTarget = positionNew;
                _positionConstantVelocityEnd += (positionNew - _positionTarget);
            }
        } else {
            // direction is negative
            if (positionNew < _positionCurrent) {
                //int positionDelta = positionNew - _positionTarget;
                _positionTarget = positionNew;
                _positionConstantVelocityEnd -= (positionNew - _positionTarget);
            }
        }
    }
}

// Called every time timer times out.
void stepper::isr(void) {
	step();
	if (_superState == MOVE_FULL || _superState == MOVE_TRUNCATED) {
		if (_directionPositive) {
			if (_positionCurrent < _positionConstantVelocityStart) {
				// accelerating
				_subState = MOVE_ACCELERATE;
				_timer(a.updateClockPeriod());
			} else if (_positionCurrent < _positionConstantVelocityEnd) {
				// constant velocity. Nothing to do.
				_subState = MOVE_CONSTANT_VELOCITY;
#if CYGWIN
                a.updateCumulativeTimeWithClockPeriod();
#endif /* CYGWIN */                
			} else if (_positionCurrent == _positionConstantVelocityEnd) {
				// Start of deceleration.
				_subState = MOVE_DECELERATE;
				a.initAccelTime(a.accelTime());
				_timer(a.updateClockPeriodReverse());
			} else if (_positionCurrent < _positionTarget) {
				// decelerating
				_subState = MOVE_DECELERATE;
				_timer(a.updateClockPeriodReverse());
			} else /* if (_positionCurrent >= _positionTarget) */ {
				// end of movement
				_timerStart(false);
			}
		} else {
			if (_positionCurrent > _positionConstantVelocityStart) {
				// accelerating
				_subState = MOVE_ACCELERATE;
				_timer(a.updateClockPeriod());
			} else if (_positionCurrent > _positionConstantVelocityEnd) {
				// constant velocity. Nothing to do.
				_subState = MOVE_CONSTANT_VELOCITY;
#if CYGWIN
                a.updateCumulativeTimeWithClockPeriod();
#endif /* CYGWIN */                
			} else if (_positionCurrent == _positionConstantVelocityEnd) {
				// Start of deceleration.
				_subState = MOVE_DECELERATE;
				a.initAccelTime(a.accelTime());
				_timer(a.updateClockPeriodReverse());
			} else if (_positionCurrent > _positionTarget) {
				// decelerating
				_subState = MOVE_DECELERATE;
				_timer(a.updateClockPeriodReverse());
			} else /* if (_positionCurrent <= _positionTarget) */ {
				// end of movement
				_timerStart(false);
			}
		}
	} else if (_superState == VELOCITY_MOVE) {
		if (_directionPositive) {
			if (_positionCurrent < _positionConstantVelocityStart) {
				// accelerating
				_timer(a.updateClockPeriod());
			} else if (_positionCurrent == _positionConstantVelocityStart) {
				if (a.freqCloseToStop(a.freqFromClockTicks(_timer()))) {
					// end of movement
					_timerStart(false);
				} else {
					_subState = VELOCITY_MOVE_CONSTANT_VELOCITY;
				}
			// } else if (_positionCurrent >= _positionConstantVelocityStart) { // compiler could optimize this line out, but we'll do it just to be explicit
				// constant velocity. Nothing to do.
			}
		} else {
			if (_positionCurrent > _positionConstantVelocityStart) {
				// accelerating
				_timer(a.updateClockPeriod());
			} else if (_positionCurrent == _positionConstantVelocityStart) {
				if (a.freqCloseToStop(a.freqFromClockTicks(_timer()))) {
					// end of movement
					_timerStart(false);
				} else {
					_subState = VELOCITY_MOVE_CONSTANT_VELOCITY;
				}
			// } else if (_positionCurrent <= _positionConstantVelocityStart) {
				// constant velocity. Nothing to do.
			}
		}
	}

#if REGRESS_2
#if CYGWIN
    switch (_superState) {
    case IDLE:
        oss() << "IDLE ";
        break;
    case MOVE_FULL:
        oss() << "MOVE_FULL ";
        break;
    case MOVE_TRUNCATED:
        oss() << "MOVE_TRUNCATED ";
        break;
    case VELOCITY_MOVE:
        oss() << "VELOCITY_MOVE ";
        break;
    default:
        oss() << "UNKNOWN SUPERSTATE ";
        break;
    }
#else /* not CYGWIN */
    char isrBuf[32];
    if (_superState != _superStateLast) {
        switch (_superState) {
        case IDLE:
            sprintf(isrBuf, "<I>");
            break;
        case MOVE_FULL:
            sprintf(isrBuf, "<MF>");
            break;
        case MOVE_TRUNCATED:
            sprintf(isrBuf, "<MT>");
            break;
        case VELOCITY_MOVE:
            sprintf(isrBuf, "<VM>");
            break;
        default:
            sprintf(isrBuf, "<UNK%d>", _superState);
            break;
        }
        //UARTSend((unsigned char *)isrBuf, strlen(isrBuf));
        _superStateLast = _superState;
    }
#endif /* CYGWIN */

#if CYGWIN    
    switch (_subState) {
    case MOVE_START:
        oss() << "MOVE_START ";
        break;
    case MOVE_ACCELERATE:
        oss() << "MOVE_ACCELERATE ";
        break;
    case MOVE_CONSTANT_VELOCITY:
        oss() << "MOVE_CONSTANT_VELOCITY ";
        break;
    case MOVE_DECELERATE:
        oss() << "MOVE_DECELERATE ";
        break;
    case VELOCITY_MOVE_ACCELERATE:
        oss() << "VELOCITY_MOVE_ACCEL ";
        break;
    case VELOCITY_MOVE_DECELERATE:
        oss() << "VELOCITY_MOVE_DECEL ";
        break;
    case VELOCITY_MOVE_CONSTANT_VELOCITY:
        oss() << "VELOCITY_MOVE_CONSTANT_VELOCITY ";
        break;
    default:
        oss() << "UNKNOWN SUBSTATE ";
        break;
    }
    oss() << " position=" << _positionCurrent << " timer=" << _timer();
    // This line prints total elapsed time vs frequency (speed)
    oss() << "  us vs. frequency: " << a.clockTicksToMicroSec(a.cumulativeClockTicks()) << " " << a.freqFromClockTicks(a.currentClockTicks());
    dump();
#else /* not CYGWIN */
    if (_subState != _subStateLast) {
        switch (_subState) {
        case MOVE_START:
            sprintf(isrBuf, "<ms>");
            break;
        case MOVE_ACCELERATE:
            sprintf(isrBuf, "<ma>");
            break;
        case MOVE_CONSTANT_VELOCITY:
            sprintf(isrBuf, "<mcv%d>", _timer());
            break;
        case MOVE_DECELERATE:
            sprintf(isrBuf, "<md>");
            break;
        case VELOCITY_MOVE_ACCELERATE:
            sprintf(isrBuf, "<vma>");
            break;
        case VELOCITY_MOVE_DECELERATE:
            sprintf(isrBuf, "<vmd>");
            break;
        case VELOCITY_MOVE_CONSTANT_VELOCITY:
            sprintf(isrBuf, "<vmcv>");
            break;
        default:
            sprintf(isrBuf, "<?%d>", _subState);
            break;
        }    
        //UARTSend((unsigned char *)isrBuf, strlen(isrBuf));
        _subStateLast = _subState;
    }
#endif /* CYGWIN */
    
#endif /* REGRESS_2 */				
}

void stepper::_timerStart(bool start /* = true */) {
	if (start) {
#if CYGWIN    
        a.initCumulativeTime();
#endif /* CYGWIN */        
        enable();
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
