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
#define SECONDS_PER_MINUTE      (60)
#define DEGREES_PER_REV         (360)
#define DEGREES_PER_REV_TIMES_MINUTE_PER_SEC    (6) // 360 * 0.01666 = 6
#define DEGREES_PER_STEP_X10000 (1125)  // See comment for _degPerStepX10000 below.
#define STEP_PER_DEGREE         (16.0 / 1.8)    // Assuming 16-microsteps, 8.889 
#define ACCEL_SHARPNESS_MIN     (1)
#define ACCEL_SHARPNESS_DEFAULT (8)
#define ACCEL_SHARPNESS_MAX     (32)

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
	  
    void initAccelTime(const unsigned int t) {
	    _totalClockTicks = 0;
		time(t);
		_currentClockTicks = clockTicks(0);
	}
	// set/get acceleration time
    void time(const unsigned int us) {_time = us;}
    unsigned int time(void) {return _time;}
	
    unsigned int dryRunAccel(void);
	void frequency(const unsigned int fmin = 200, const unsigned int fmax = 1200);
    void RPMx10k(const unsigned int RPMx10kmin = 1, const unsigned int RPMx10kmax = 1000);
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

	bool freqCloseToStop(unsigned int f) {
		if (f <= _fStop) {
			return true;
		}
		return false;
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
    void _initUnitCurve(int sharpness = ACCEL_SHARPNESS_DEFAULT);
    void _scaleYAxisToFrequency(void);
#if OPTIMIZE_CURVE_CALC	
	void _scaleYAxisToClockTicks(void);
#else /* not OPTIMIZE_CURVE_CALC */
    void _scaleYAxisToMicroSec(void);
    void _scaleYAxisToClockTicks(void);
#endif /* OPTIMIZE_CURVE_CALC */	
    
	// Given clock ticks, return equivalent microsec.
    unsigned int _clockTicksToMicroSec(const unsigned int ct) {
		return (unsigned int)((ct / _clockMHz));
	}
	
    unsigned int _RPMx10ktoFreq(unsigned int RPMx10k) {
        return RPMx10k / SECONDS_PER_MINUTE * DEGREES_PER_REV * _stepPerDegree;
    }
    
#if DUMP	
	void _dumpCurveFloat(void);
	void _dumpCurveInt(void);
#endif /* DUMP */
	
    //enum {_maxAccelIndex = 99, _maxAccelEntries, _maxBisectionTrys = 20};
    float _curveFloat[_maxAccelEntries];
    unsigned int _curveInt[_maxAccelEntries];
    
    //int _positionCurrent;  // in steps
    unsigned int _totalClockTicks; 
    unsigned int _currentClockTicks;    // period of current timer interrupt
    unsigned int _time;			// acceleration time in us
    //int _acceleration;          // 0 when constant velocity
    //int _velocity;              // step/s
	unsigned int _fmin, _fmax;	// frequency min/max for acceleration

    const unsigned int _microSecPerSec;
    const unsigned int _maxDryRunCycles;
    const float _clockMHz;
	const unsigned int _minTime;
	const unsigned int _maxTime;
	const unsigned int _fStop;	// Frequency that we can assume the motor is as good as stopped
    // degrees per step x10000. Therefore, 1.8 x10000 = 18000 deg/step
    // A lot of steppers are 1.8 deg/step, however, with 16 microsteps per
    //  full step this is 1.8 / 16 = 0.1125 deg/step x10000 = 1125.
    //unsigned int _degPerStepX10000;
    float _stepPerDegree;
};

#endif /* _ACCEL_H_ */