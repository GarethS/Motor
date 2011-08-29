/*
	Copyright (c) Gareth Scott 2011

	accle.cpp 

*/

#include "accel.h"
#include <math.h>

accel::accel() :
#if CYGWIN 
					logc(std::string("ACCEL")),
#endif /* CYGWIN */					
					_time(1000000),
					_microSecPerSec(1000000), _maxDryRunCycles(10000), _clockMHz(8.0),
					_minTime(1000), _maxTime(4000000), _fStop(200) {
	frequency();	// set initial min/max frequency (speed) curve
}

accel::accel(const accel& a) :
#if CYGWIN
    logc(a),
#endif /* CYGWIN */
    _microSecPerSec(a._microSecPerSec), _maxDryRunCycles(a._maxDryRunCycles), _clockMHz(a._clockMHz), _minTime(a._minTime), _maxTime(a._maxTime), _fStop(a._fStop) {
	assign(a);
}

accel& accel::operator=(const accel& a) {
	if (this != &a) {
		assign(a);
	}
	return *this;
}

void accel::assign(const accel& a) {
	*(struct passFloatArray*)_curveFloat = *(struct passFloatArray*)a._curveFloat;	// Beats copying each individual element
	*(struct passIntArray*)_curveInt = *(struct passIntArray*)a._curveInt;
	_positionCurrent = a._positionCurrent;
	_totalClockTicks = a._totalClockTicks;
	_currentClockTicks = a._currentClockTicks;
	_time = a._time;
	_acceleration = a._acceleration;
	_velocity = a._velocity;
	_fmin = a._fmin;
	_fmax = a._fmax;
	
}

void accel::test(void) {
#if CYGWIN
	oss() << "_clockTicksToMicroSec START" << endl;
	for (int ct = 0; ct < 100; ++ct) {
		unsigned int microsec = _clockTicksToMicroSec(ct);
		oss() << "ct=" << ct << " microsec=" << microsec << endl;
	}
	oss() << "_clockTicksToMicroSec END" << endl;
	dump();
	
	oss() << "freqFromTime START" << endl;
	for (int us = 0; us < 100; ++us) {
		unsigned int f = freqFromTime(us);
		oss() << "us=" << us << " freq=" << f << endl;
	}
	oss() << "freqFromTime END" << endl;
	dump();
	
	oss() << "freqFromClockTicks START" << endl;
	for (int ct = 0; ct < 100; ++ct) {
		unsigned int f = freqFromClockTicks(ct);
		oss() << "ct=" << ct << " freq=" << f << endl;
	}
	oss() << "freqFromClockTicks END" << endl;
	dump();
	
	oss() << "_time=" << time() << endl;
	oss() << "clockTicksToCurveIndex START" << endl;
	for (int ct = 0; ct < 8000000; ct += 0x8000) {
		unsigned int ci = clockTicksToCurveIndex(ct);
		oss() << "ct=" << ct << " curveIndex=" << ci << endl;
	}
	oss() << "clockTicksToCurveIndex END" << endl;
	dump();
	
	oss() << "_time=" << time() << endl;
	oss() << "clockTicksToCurveIndexReverse START" << endl;
	for (int ct = 0; ct < 8000000; ct += 0x8000) {
		unsigned int ci = clockTicksToCurveIndexReverse(ct);
		oss() << "ct=" << ct << " curveIndex=" << ci << endl;
	}
	oss() << "clockTicksToCurveIndexReverse END" << endl;
	dump();
#endif /* CYGWIN */
}

unsigned int accel::dryRunAccel(void) {
    primeTime(time());
    unsigned int step = 0;
#if DUMP
	cout << "start: dryRunAccel" << endl;
#endif /* DUMP */	
    for (; step < _maxDryRunCycles; ++step) {
		// Each step through is equivalent to an isr call from the timer.
        _totalClockTicks += _currentClockTicks;
        unsigned int index = clockTicksToCurveIndex(_totalClockTicks);
#if DUMP
		cout << "index=" << index << " step=" << step << " freqFromClockTicks=" << freqFromClockTicks(_currentClockTicks) << endl;
		ostringstream oss;
		oss << freqFromClockTicks(_currentClockTicks);
		//oss << " step= " << step << " freq= " << freqFromClockTicks(_currentClockTicks);
		dump(oss.str(), false);
		//cout << "index=" << index << " step=" << step << " _totalClockTicks=" << _totalClockTicks << endl;
#endif /* DUMP */		
        if (index >= _maxAccelIndex) {
            break;
        }
        _currentClockTicks = clockTicks(index);
    }
#if DUMP
	cout << "stop: dryRunAccel" << endl;
#endif /* DUMP */	
    return step;
}

// Very similar to dryRunAccel() above but without debug output. Given an acceleration time, how many steps are taken.
unsigned int accel::timeToSteps(const unsigned int t) {
    primeTime(t);
    unsigned int step = 0;
    for (; step < _maxDryRunCycles; ++step) {
		// Each step through is equivalent to an isr call from the timer.
        _totalClockTicks += _currentClockTicks;
        unsigned int index = clockTicksToCurveIndex(_totalClockTicks);
        if (index >= _maxAccelIndex) {
            break;
        }
        _currentClockTicks = clockTicks(index);
    }
    return step;
}

// Returns acceleration time given steps required. Uses a bisection algorithm to home in on the solution.
unsigned int accel::stepsToTime(const unsigned int steps) {
	unsigned int maxAccelTime = _maxTime;
	unsigned int minAccelTime = _minTime;
	unsigned int currentAccelTime = (maxAccelTime + minAccelTime) / 2;
	unsigned int actualSteps;
	unsigned int originalTime = time();
	
#if REGRESS_1
	oss() << "START" << endl;
#endif /* REGRESS_1 */
	for (int i = 0; i < _maxBisectionTrys; ++i) {
		actualSteps = timeToSteps(currentAccelTime);
#if REGRESS_1
		oss() << "currentAccelTime= " << currentAccelTime << " actualSteps= " << actualSteps << endl;
#endif /* REGRESS_1 */
		if (actualSteps > steps) {
			// reduce currentAccelTime
			maxAccelTime = currentAccelTime;
		} else if (actualSteps < steps) {
			// increase currentAccelTime
			minAccelTime = currentAccelTime;
		} else {
			break;
		}
		currentAccelTime = (maxAccelTime + minAccelTime) / 2;
	}
#if REGRESS_1
	oss() << "END currentAccelTime= " << currentAccelTime << " actualSteps= " << actualSteps;
	dump();
#endif /* REGRESS_1 */
	time(originalTime);
	return currentAccelTime;
}

#if REGRESS_1
void accel::_dumpCurveFloat(void) {
    for (int i = 0; i < _maxAccelEntries; ++i) {
		oss() << i << " " << _curveFloat[i] << endl;
    }
	dump();
}

void accel::_dumpCurveInt(void) {
    for (int i = 0; i < _maxAccelEntries; ++i) {
		oss() << i << " " << _curveInt[i] << endl;
    }
	dump();
}
#endif /* REGRESS_1 */

// Acceleration curve is plotted on x-axis between -sharp and sharp (e.g. -8 to 8)
//  The s-curve is defined as: 1/(1 + e^-x)
void accel::_initUnitCurve(const int sharpness /* = 8 */) {
    float step = sharpness * 2.0 / _maxAccelIndex;
    float x = -sharpness;
    for (int i = 0; i < _maxAccelEntries; ++i, x += step) {
        _curveFloat[i] = 1 / (1 + exp(-x));
    }
#if REGRESS_1
    oss() << "sharpness=" << x << " step=" << step << endl;
	oss() << "unit curve" << endl;
	_dumpCurveFloat();
#endif /* REGRESS_1 */
}

// Scale unit acceleration curve on y-axis using the line: y = mx + b. 
//  Since unit acceleration is from 0 - 1, m = fmax - fmin (i.e. 1200 - 200 = 1000) and
//  b = 200
//  End result is unit curve scaled only on y-axis where y represents frequency. 
//  Frequency is steps (actually micro-steps) per sec.
void accel::_scaleYAxisToFrequency(void) {
    float m = _fmax - _fmin;
    float b = _fmin;
    for (int i = 0; i < _maxAccelEntries; ++i) {
        _curveFloat[i] = (_curveFloat[i] * m) + b;
    }
#if REGRESS_1
	oss() << "_scaleYAxisToFrequency" << endl;
	_dumpCurveFloat();
#endif /* REGRESS_1 */
}

#if OPTIMIZE_CURVE_CALC
void accel::_scaleYAxisToClockTicks(void) {
	unsigned int clockFrequency = (unsigned int)(_clockMHz * _microSecPerSec);
    for (int i = 0; i < _maxAccelEntries; ++i) {
        _curveInt[i] = (unsigned int)(clockFrequency / _curveFloat[i]);
    }
#if REGRESS_1
	oss() << "_scaleYAxisToClockTicks" << endl;
	_dumpCurveInt();
#endif /* REGRESS_1 */
} 

#else /* not OPTIMIZE_CURVE_CALC */

// Convert frequency scaled curve to period in microsec (us).
void accel::_scaleYAxisToMicroSec(void) {
    for (int i = 0; i < _maxAccelEntries; ++i) {
        _curveInt[i] = _microSecPerSec / _curveFloat[i];
    }
#if REGRESS_1
	oss() << "_scaleYAxisToMicroSec" << endl;
	_dumpCurveInt();
#endif /* REGRESS_1 */
}

void accel::_scaleYAxisToClockTicks(void) {
    for (int i = 0; i < _maxAccelEntries; ++i) {
        _curveInt[i] *= _clockMHz;
    }
#if REGRESS_1
	oss() << "_scaleYAxisToClockTicks" << endl;
	_dumpCurveInt();
#endif /* REGRESS_1 */
} 
#endif /* OPTIMIZE_CURVE_CALC */

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
#if OPTIMIZE_CURVE_CALC	
	_scaleYAxisToClockTicks();
#else
	_scaleYAxisToMicroSec();
	_scaleYAxisToClockTicks();
#endif /* OPTIMIZE_CURVE_CALC */	
}
