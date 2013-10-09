/*
	Copyright (c) Gareth Scott 2011, 2012, 2013

	accel.h 

	Generate acceleration curve at construction time given 3 variables: min frequency, max frequency and time.
	
*/

#ifndef _ACCEL_H_
#define _ACCEL_H_

#if CYGWIN
#include "log.h"
#include <sstream>

using namespace std;
#endif /* CYGWIN */

#include <limits.h>

#define SECONDS_PER_MINUTE      (60)
#define MICROSEC_PER_SEC        (1000000)
#define MHZ                     (MICROSEC_PER_SEC)
#define DEGREES_PER_REV         (360)
#define DEGREES_PER_REV_TIMES_MINUTE_PER_SEC    (6) // 360 * 0.01666 = 6
#define DEGREES_PER_MICROSTEP_NOMINAL   (1.8)   // A lot of stepper motors are 1.8 degrees per full-step
#define ACCEL_SHARPNESS_MIN     (1)
#define ACCEL_SHARPNESS_DEFAULT (8)
#define ACCEL_SHARPNESS_MAX     (32)

#define MAX_DRY_RUN_CYCLES      (10000)

// _maxAccelIndex = 199 gives very smooth acceleration
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
#if CYGWIN
        //_cumulativeClockTicks = 0;
#endif /* CYGWIN */        
		accelTime(t);
		_currentClockTicks = curveIndexToClockTicks(0);
	}
    
#if CYGWIN    
    void initCumulativeTime(void) {_cumulativeClockTicks = 0;}
#endif /* CYGWIN */
    
	// set/get acceleration time
    void accelTime(const unsigned int us) {_accelTime = us;}
    unsigned int accelTime(void) const {return _accelTime;}
	
    unsigned int dryRunAccel(void);
	void frequency(const unsigned int fmin = 200, const unsigned int fmax = 3200);
    // Convert RPM to frequency
    void RPMx10k(const unsigned int RPMx10kmin, const unsigned int RPMx10kmax) {frequency(_RPMx10ktoFreq(RPMx10kmin), _RPMx10ktoFreq(RPMx10kmax));}
    unsigned int fmin(void) const {return _fmin;}
    unsigned int fmax(void) const {return _fmax;}
	
	unsigned int curveIndexToClockTicks(const unsigned int index) {return _curveInt[index];}
	unsigned int timeToSteps(const unsigned int t);
	unsigned int stepsToTime(const unsigned int steps);
	
	unsigned int updateClockPeriod(void) {
		_totalClockTicks += _currentClockTicks;
#if CYGWIN
        // Used for debugging to get a profile of time vs frequency (speed)
        _cumulativeClockTicks += _currentClockTicks;
#endif /* CYGWIN */        
		unsigned int index = clockTicksToCurveIndex(_totalClockTicks);
		/* 
		if (index >= _maxAccelEntries - 1) {
			// Can't get here.
			return 0xffffffff;
		}
		*/
		_currentClockTicks = curveIndexToClockTicks(index);
		return _currentClockTicks;
	}

#if CYGWIN	
    unsigned int updateCumulativeTimeWithClockPeriod(void) {
        _cumulativeClockTicks += _currentClockTicks;
        return _cumulativeClockTicks;
    }
#endif /* CYGWIN */    
    
	// Used for deceleration where the curve is traversed backwards (i.e. right to left)
	unsigned int updateClockPeriodReverse(void) {
		_totalClockTicks += _currentClockTicks;
#if CYGWIN
        _cumulativeClockTicks += _currentClockTicks;
#endif /* CYGWIN */        
		unsigned int index = clockTicksToCurveIndexReverse(_totalClockTicks);
		_currentClockTicks = curveIndexToClockTicks(index);
		return _currentClockTicks;
	}
	
	unsigned int clockTicksToCurveIndex(const unsigned int ct) const {
	    return microSecToCurveIndex(clockTicksToMicroSec(ct));
	}
	unsigned int clockTicksToCurveIndexReverse(const unsigned int ct) const {
	    return microSecToCurveIndexReverse(clockTicksToMicroSec(ct));
	}

	bool freqCloseToStop(const unsigned int f) const {
		if (f <= _fStop) {
			return true;
		}
		return false;
	}
	
    // convert beween: frequency <-> clockTicks
    unsigned int freqToClockTicks(const unsigned int f) const {
        return microSecToClockTicks(freqToMicroSec(f));
    }
	unsigned int clockTicksToFreq(const unsigned int ct) const {
        return microSecToFreq(clockTicksToMicroSec(ct));
	}

    // convert beween: microseconds <-> frequency
	unsigned int microSecToFreq(const unsigned int us) const {
		if (us == 0) {/* Avoid divide-by-zero */return INT_MAX;} return MICROSEC_PER_SEC / us;
	}
    unsigned int freqToMicroSec(const unsigned int f) const {
        return microSecToFreq(f);   // This may look wrong, but it's correct. Frequency is the inverse of period (i.e. microseconds) and vice versa.
    }

    // convert beween: clockTicks <-> microseconds
    unsigned int clockTicksToMicroSec(const unsigned int ct) const {
		return (unsigned int)(ct / _clockMHz);
	}
    unsigned int microSecToClockTicks(const unsigned int us) const {
        return (unsigned int)(us * _clockMHz);
    }
	
    // Given microsec, return acceleration curve index to get speed
	unsigned int microSecToCurveIndex(const unsigned int us) const {
		if (us > accelTime()) {
			return _maxAccelIndex;
		}
#if 0
		unsigned int index = us * _maxAccelEntries / accelTime();
		cout << "microSecToCurveIndex: index=" << index << endl;
#endif /* DUMP */
		return us * _maxAccelIndex / accelTime();
	}
	// Same as microSecToCurveIndex(), except returns curve index in reverse order. 
	//  Used for deceleration.
	unsigned int microSecToCurveIndexReverse(const unsigned int us) const {
		if (us > accelTime()) {
			return 0;
		}
		return _maxAccelIndex - (us * _maxAccelEntries / accelTime());
	}

    void degreesPerMicrostepx10k(const unsigned int dpus) {_degreesPerMicrostepx10k = dpus;}
    unsigned int degreesPerMicrostepx10k(void) const  {return _degreesPerMicrostepx10k;}
    
#if CYGWIN    
    unsigned int currentClockTicks(void) const {return _currentClockTicks;}
    unsigned int cumulativeClockTicks(void) const {return _cumulativeClockTicks;}
#endif /* CYGWIN */    
    
	void test(void);
    
private:
    void _initUnitCurve(const int sharpness = ACCEL_SHARPNESS_DEFAULT);
    void _scaleYAxisToFrequency(void);
	void _scaleYAxisToClockTicks(void);
    
    // Avoid using floats
    unsigned int _RPMx10ktoFreq(const unsigned int RPMx10k) {
#if REGRESS_1
        oss() << "_RPMx10ktoFreq rpmx10k:" << RPMx10k << " frequency:" << (RPMx10k * DEGREES_PER_REV_TIMES_MINUTE_PER_SEC) / _degreesPerMicrostepx10k;
        dump();
#endif /* REGRESS_1 */    
        return (RPMx10k * DEGREES_PER_REV_TIMES_MINUTE_PER_SEC) / _degreesPerMicrostepx10k;
    }
    
#if DUMP	
	void _dumpCurveFloat(void);
	void _dumpCurveInt(void);
#endif /* DUMP */
	
    float _curveFloat[_maxAccelEntries];
    unsigned int _curveInt[_maxAccelEntries];
    
    unsigned int _totalClockTicks;      // accumulated time in the acceleration profile
#if CYGWIN    
    unsigned int _cumulativeClockTicks; // accumulated time in the entire velocity profile
#endif /* CYGWIN */    
    unsigned int _currentClockTicks;    // period of current timer interrupt. Inversely proportional to motor speed.
    unsigned int _accelTime;            // acceleration time in us
	unsigned int _fmin, _fmax;	        // frequency min/max for acceleration

    const float _clockMHz;
	const unsigned int _minTime;
	const unsigned int _maxTime;
	const unsigned int _fStop;	// Frequency that we can assume the motor is as good as stopped
    unsigned int _degreesPerMicrostepx10k;
};

#endif /* _ACCEL_H_ */