/*
	Copyright (c) Gareth Scott 2011

	accel.h 

	This class generates an acceleration curve at construction time given 3 variables: min frequency, max frequency and time.
	
*/

#ifndef _ACCEL_H_
#define _ACCEL_H_

#if CYGWIN
//#include <iostream>
#include "log.h"
#include <sstream>

using namespace std;
#endif /* CYGWIN */

#include <limits.h>

#define OPTIMIZE_CURVE_CALC 1

enum {_maxAccelIndex = 99, _maxAccelEntries, _maxBisectionTrys = 20};

struct passFloatArray {
    float curveFloat[_maxAccelEntries];
};

struct passIntArray {
    unsigned int curveInt[_maxAccelEntries];
};


class accel
#if CYGWIN
				: public logc
#endif /* CYGWIN */
				{
public:
    accel();
    ~accel() {}
      
	accel(const accel& a);	// copy constructor
	accel& operator=(const accel& a);
	void assign(const accel& a);
	  
    void primeTime(const unsigned int t) {
	    _totalClockTicks = 0;
		time(t);
		_currentClockTicks = clockTicks(0);
	}
	// set/get acceleration time
    void time(const unsigned int us) {_time = us;}
    unsigned int time(void) {return _time;}
	
    unsigned int dryRunAccel(void);
	void frequency(const unsigned int fmin = 200, const unsigned int fmax = 1200);
    unsigned int fmin(void) {return _fmin;}
    unsigned int fmax(void) {return _fmax;}
	
    //int acceleration(void) {/* TODO */}
    //int velocity(void) {/* TODO */}

	unsigned int clockTicks(const unsigned int index) {return _curveInt[index];}
	unsigned int timeToSteps(const unsigned int t);
	unsigned int stepsToTime(const unsigned int steps);
	
	unsigned int updateClockPeriod(void) {
		_totalClockTicks += _currentClockTicks;
		unsigned int index = clockTicksToCurveIndex(_totalClockTicks);
		/* 
		if (index >= _maxAccelEntries - 1) {
			// Can't get here.
			return 0xffffffff;
		}
		*/
		_currentClockTicks = clockTicks(index);
		return _currentClockTicks;
	}
	
	// Used for deceleration where the curve is traversed backwards (i.e. right to left)
	unsigned int updateClockPeriodReverse(void) {
		_totalClockTicks += _currentClockTicks;
		unsigned int index = clockTicksToCurveIndexReverse(_totalClockTicks);
		_currentClockTicks = clockTicks(index);
		return _currentClockTicks;
	}
	
	unsigned int clockTicksToCurveIndex(unsigned int ct) {
	    return microSecToCurveIndex(_clockTicksToMicroSec(ct));
	}
	unsigned int clockTicksToCurveIndexReverse(unsigned int ct) {
	    return microSecToCurveIndexReverse(_clockTicksToMicroSec(ct));
	}

	// Given clock ticks, return frequency
	unsigned int freqFromClockTicks(unsigned int ct) {
		if (_clockTicksToMicroSec(ct) == 0) {
			// Avoid divide-by-zero
			return INT_MAX;
		}
		return _microSecPerSec / _clockTicksToMicroSec(ct);
	}

	// Given time, return frequency
	unsigned int freqFromTime(unsigned int microsec) {
		if (microsec == 0) {
			// Avoid divide-by-zero
			return INT_MAX;
		}
		return _microSecPerSec / microsec;
	}

    // Given microsec, return acceleration curve index to get speed
	unsigned int microSecToCurveIndex(const unsigned int us) {
		if (us > time()) {
			return _maxAccelIndex;
		}
#if 0
		unsigned int index = us * _maxAccelEntries / time();
		cout << "microSecToCurveIndex: index=" << index << endl;
#endif /* DUMP */
		return us * _maxAccelEntries / time();
	}
	// Same as microSecToCurveIndex(), except returns curve index in reverse order. 
	//  Used for deceleration.
	unsigned int microSecToCurveIndexReverse(const unsigned int us) {
		if (us > time()) {
			return 0;
		}
		return _maxAccelIndex - (us * _maxAccelEntries / time());
	}

	void test(void);
    
private:
    void _initUnitCurve(const int sharpness = 8);
    void _scaleYAxisToFrequency(void);
#if OPTIMIZE_CURVE_CALC	
	void _scaleYAxisToClockTicks(void);
#else /* not OPTIMIZE_CURVE_CALC */
    void _initScaledPeriodMicroSecCurve(void);
    void _initScaledPeriodClockTicksCurve(void);
#endif /* OPTIMIZE_CURVE_CALC */	
    
	// Given clock ticks, return equivalent microsec.
    unsigned int _clockTicksToMicroSec(const unsigned int ct) {
		return (unsigned int)((ct / _clockMHz));
	}
	
#if DUMP	
	void _dumpCurveFloat(void);
	void _dumpCurveInt(void);
#endif /* DUMP */
	
    //enum {_maxAccelIndex = 99, _maxAccelEntries, _maxBisectionTrys = 20};
    float _curveFloat[_maxAccelEntries];
    unsigned int _curveInt[_maxAccelEntries];
    
    int _positionCurrent;  // in steps
    unsigned int _totalClockTicks; 
    unsigned int _currentClockTicks;
    unsigned int _time;			// acceleration time
    int _acceleration;          // 0 when constant velocity
    int _velocity;              // step/s
	unsigned int _fmin, _fmax;	// frequency min/max for acceleration

    const unsigned int _microSecPerSec;
    const unsigned int _maxDryRunCycles;
    const float _clockMHz;
	const unsigned int _minTime;
	const unsigned int _maxTime;
};

#endif /* _ACCEL_H_ */