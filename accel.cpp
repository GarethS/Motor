/*
	Copyright (c) Gareth Scott 2011, 2012, 2013

	accel.cpp 

*/

#include "accel.h"
#include "FreeRTOSConfig.h"
#include <math.h>

accel::accel() :
#if CYGWIN 
					logc(std::string("ACCEL")),
#endif /* CYGWIN */					
					_accelMicroSec(1000000), _clockMHz(configCPU_CLOCK_HZ / MHZ),
					_fStop(200), _degreesPerMicrostepx10k(DEGREES_PER_MICROSTEP_NOMINAL * 10000) {
	frequency();	// set default min/max frequency (speed) curve
}

accel::accel(const accel& a) :
#if CYGWIN
    logc(a),
#endif /* CYGWIN */
    _clockMHz(a._clockMHz), _fStop(a._fStop),
    _degreesPerMicrostepx10k(DEGREES_PER_MICROSTEP_NOMINAL * 10000) {
	assign(a);
}

accel& accel::operator=(const accel& a) {
	if (this != &a) {
		assign(a);
	}
	return *this;
}

void accel::assign(const accel& a) {
	*(struct passFloatArray*)_curveFreq = *(struct passFloatArray*)a._curveFreq;	// Beats copying each individual element
	*(struct passIntArray*)_curveClockTicks = *(struct passIntArray*)a._curveClockTicks;
	_totalClockTicks = a._totalClockTicks;
	_currentClockTicks = a._currentClockTicks;
	_accelMicroSec = a._accelMicroSec;
	_fmin = a.fmin();
	_fmax = a.fmax();
	_degreesPerMicrostepx10k = a._degreesPerMicrostepx10k;
}

void accel::test(void) {
#if CYGWIN
	oss() << "_clockTicksToMicroSec START" << endl;
	for (int ct = 0; ct < 100; ++ct) {
		unsigned int microsec = clockTicksToMicroSec(ct);
		oss() << "ct=" << ct << " microsec=" << microsec << endl;
	}
	oss() << "_clockTicksToMicroSec END" << endl;
	dump();
	
	oss() << "freqFromTime START" << endl;
	for (int us = 0; us < 100; ++us) {
		unsigned int f = microSecToFreq(us);
		oss() << "us=" << us << " freq=" << f << endl;
	}
	oss() << "freqFromTime END" << endl;
	dump();
	
	oss() << "freqFromClockTicks START" << endl;
	for (int ct = 0; ct < 100; ++ct) {
		unsigned int f = clockTicksToFreq(ct);
		oss() << "ct=" << ct << " freq=" << f << endl;
	}
	oss() << "freqFromClockTicks END" << endl;
	dump();
	
	oss() << "_accelTime=" << accelMicroSec() << endl;
	oss() << "clockTicksToCurveIndex START" << endl;
	for (int ct = 0; ct < 8000000; ct += 0x8000) {
		unsigned int ci = clockTicksToCurveIndex(ct);
		oss() << "ct=" << ct << " curveIndex=" << ci << endl;
	}
	oss() << "clockTicksToCurveIndex END" << endl;
	dump();
	
	oss() << "_accelTime=" << accelMicroSec() << endl;
	oss() << "clockTicksToCurveIndexDecelerate START" << endl;
	for (int ct = 0; ct < 8000000; ct += 0x8000) {
		unsigned int ci = clockTicksToCurveIndexDecelerate(ct);
		oss() << "ct=" << ct << " curveIndex=" << ci << endl;
	}
	oss() << "clockTicksToCurveIndexDecelerate END" << endl;
	dump();
#endif /* CYGWIN */
}

unsigned int accel::dryRunAccel(void) {
    reset(accelMicroSec());
    unsigned int step = 0;
#if DUMP
	oss() << "start: dryRunAccel" << endl;
#endif /* DUMP */	
    for (; step < MAX_DRY_RUN_CYCLES; ++step) {
		// Each step through is equivalent to an isr call from the timer.
        _totalClockTicks += _currentClockTicks;
        unsigned int index = clockTicksToCurveIndex(_totalClockTicks);
#if DUMP
		//cout << "index=" << index << " step=" << step << " freqFromClockTicks=" << clockTicksToFreq(_currentClockTicks) << "time(us)=" << _clockTicksToMicroSec(_totalClockTicks) << endl;
		//oss() << "index=" << index << " step=" << step << " freqFromClockTicks=" << clockTicksToFreq(_currentClockTicks) << " cummulative time(us)=" << _clockTicksToMicroSec(_totalClockTicks);
        // This next line prints out 2 columns that can be plotted in a spreadsheet showing total time vs frequency
		oss() << clockTicksToFreq(_currentClockTicks) << " " << clockTicksToMicroSec(_totalClockTicks);
        dump();
		//ostringstream oss;
		//oss << clockTicksToFreq(_currentClockTicks);
		//oss << " step= " << step << " freq= " << clockTicksToFreq(_currentClockTicks);
		//dump(oss.str(), false);
		//cout << "index=" << index << " step=" << step << " _totalClockTicks=" << _totalClockTicks << endl;
#endif /* DUMP */		
        if (index >= _maxAccelIndex) {
            break;
        }
        _currentClockTicks = curveIndexToClockTicks(index);
    }
#if DUMP
	oss() << "stop: dryRunAccel";
    dump();
#endif /* DUMP */	
    return step;
}

// Very similar to dryRunAccel() above but without debug output. Given an acceleration time, how many steps are taken.
unsigned int accel::microSecToSteps(const unsigned int us) {
    // Assert we're not in isr
    reset(us);
    unsigned int step = 0;
    for (; step < MAX_DRY_RUN_CYCLES; ++step) {
		// Each step through is equivalent to an isr call from the timer.
        _totalClockTicks += _currentClockTicks;
        if (clockTicksToMicroSec(totalClockTicks()) > us) {
            break;
        }
        unsigned int index = clockTicksToCurveIndex(_totalClockTicks);
        if (index >= _maxAccelIndex) {
            break;
        }
        _currentClockTicks = curveIndexToClockTicks(index);
    }
    return step;
}

// Returns acceleration time given steps required. Uses a bisection algorithm to home in on solution.
unsigned int accel::stepsToMicroSec(const unsigned int steps) {
    // Add assertion we're not in isr
	unsigned int maxAccelTime = ACCEL_MAX_MICROSEC;
	unsigned int minAccelTime = ACCEL_MIN_MICROSEC;
	unsigned int currentAccelMicroSec = (ACCEL_MAX_MICROSEC + ACCEL_MIN_MICROSEC) / 2;
	unsigned int actualSteps;
	unsigned int originalMicroSec = accelMicroSec();
	
#if REGRESS_1
	oss() << "START" << endl;
#endif /* REGRESS_1 */
	for (int i = 0; i < _maxBisectionTrys; ++i) {
		actualSteps = microSecToSteps(currentAccelMicroSec);
#if REGRESS_1
		oss() << "currentAccelMicroSec: " << currentAccelMicroSec << " actualSteps: " << actualSteps << endl;
#endif /* REGRESS_1 */
		if (actualSteps > steps) {
			// reduce currentAccelTime
			maxAccelTime = currentAccelMicroSec;
		} else if (actualSteps < steps) {
			// increase currentAccelTime
			minAccelTime = currentAccelMicroSec;
		} else {
			break;
		}
		currentAccelMicroSec = (maxAccelTime + minAccelTime) / 2;
	}
#if REGRESS_1
	oss() << "END Final currentAccelMicroSec: " << currentAccelMicroSec << " actualSteps: " << actualSteps;
	dump();
#endif /* REGRESS_1 */
	accelMicroSec(originalMicroSec);
	return currentAccelMicroSec;
}

#if REGRESS_1
void accel::_dumpCurveFreq(void) {
    for (int i = 0; i < _maxAccelEntries; ++i) {
		oss() << i << " " << _curveFreq[i] << endl;
    }
	dump();
}

void accel::_dumpCurveClockTicks(void) {
    for (int i = 0; i < _maxAccelEntries; ++i) {
		oss() << i << " " << _curveClockTicks[i] << endl;
    }
	dump();
}

#if ACCEL_LINEAR_FIT
void accel::_dumpLinearInterpolate(void) {
    for (int i = 0; i < _maxAccelEntries; ++i) {
		oss() << i << " " << _linearInterpolate[i] << endl;
    }
	dump();
}
#endif // ACCEL_LINEAR_FIT
#endif /* REGRESS_1 */

// Acceleration curve is plotted on x-axis between -sharp and sharp (e.g. -8 to 8)
//  The s-curve is defined as: 1/(1 + e^-x)
// Larger number moves closer to step function. 32 is a very sharp step.
// Lower number is close to a linear ramp. 1 is very close to a 45 deg line.
// A sharpness of 8 gives a nicely formed S-curve.
void accel::_initUnitCurve(int sharpness /* = ACCEL_SHARPNESS_DEFAULT */) {
    if (sharpness < ACCEL_SHARPNESS_MIN) {
        sharpness = ACCEL_SHARPNESS_MIN;
    } else if (sharpness > ACCEL_SHARPNESS_MAX) {
        sharpness = ACCEL_SHARPNESS_MAX;
    }
    float step = sharpness * 2.0 / _maxAccelIndex;
    float x = -sharpness;
    for (int i = 0; i < _maxAccelEntries; ++i, x += step) {
        _curveFreq[i] = 1 / (1 + exp(-x));
    }
#if REGRESS_1
    oss() << "sharpness=" << x << " step=" << step << endl;
	oss() << "unit curve" << endl;
	_dumpCurveFreq();
#endif /* REGRESS_1 */
}

// Scale unit acceleration curve on y-axis using the line: y = mx + b. 
//  Since unit acceleration is from 0 - 1, m = fmax - fmin (i.e. 1200 - 200 = 1000) and
//  b = 200
//  End result is unit curve scaled only on y-axis where y represents frequency. 
//  Frequency is steps (actually micro-steps) per sec.
//  TODO: Actually, as sharpness decreases in _initUnitCurve(), the acceleration is not from 0-1, but inside that range.
//   Need to find min/max values and adjust accordingly.
void accel::_scaleYAxisToFrequency(void) {
    float m = (fmax() - fmin()) / (_curveFreq[_maxAccelIndex] - _curveFreq[0]); 
    float b = fmin() - (m * _curveFreq[0]);
    for (int i = 0; i < _maxAccelEntries; ++i) {
        _curveFreq[i] = (_curveFreq[i] * m) + b;
    }
#if ACCEL_LINEAR_FIT    
    unsigned int microSecStep = _accelMicroSecStep();
    for (int i = 0; i < _maxAccelEntries - 1; ++i) {
        _linearInterpolate[i] = (_curveFreq[i+1] - _curveFreq[i]) / microSecStep;
    }
#endif // ACCEL_LINEAR_FIT    
#if REGRESS_1
	oss() << "_scaleYAxisToFrequency" << endl;
	_dumpCurveFreq();
#endif /* REGRESS_1 */
}

void accel::_scaleYAxisToClockTicks(void) {
	unsigned int clockFrequency = (unsigned int)(_clockMHz * MICROSEC_PER_SEC);
    for (int i = 0; i < _maxAccelEntries; ++i) {
        _curveClockTicks[i] = (unsigned int)(clockFrequency / _curveFreq[i]);
    }
#if REGRESS_1
	oss() << "_scaleYAxisToClockTicks" << endl;
	_dumpCurveClockTicks();
#endif /* REGRESS_1 */
} 

// Frequency is step per second but may want to increase this to step/sec * 100000 to get
//  low RPM
void accel::frequency(const unsigned int fmin /* = 200 */, const unsigned int fmax /* = 1200 */) {
	if (fmin > fmax) {
		// fmax should always be higher than fmin
		_fmin = fmax;
		_fmax = fmin;
	} else {
		_fmin = fmin;
		_fmax = fmax;
	}
    _initUnitCurve();
	_scaleYAxisToFrequency();
	_scaleYAxisToClockTicks();
}
