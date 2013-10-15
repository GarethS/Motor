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
#include <iomanip>

using namespace std;
#endif /* CYGWIN */

#include <limits.h>
#include <assert.h>

#define ACCEL_LINEAR_FIT        0   // Do linear interpolated acceleration between points on the acceleration S-curve

#define MICROSEC_PER_SEC        (1000000)
#define MHZ                     (MICROSEC_PER_SEC)
#define DEGREES_PER_REV         (360)
#define DEGREES_PER_REV_TIMES_MINUTE_PER_SEC    (6) // 360 * 0.01666 = 6
#define DEGREES_PER_MICROSTEP_NOMINAL   (1.8)   // A lot of stepper motors are 1.8 degrees per full-step
#define ACCEL_SHARPNESS_MIN     (1)
#define ACCEL_SHARPNESS_DEFAULT (8)
#define ACCEL_SHARPNESS_MAX     (32)
#define ACCEL_MIN_MICROSEC      (1000)
#define ACCEL_MAX_MICROSEC      (4000000)

#define MAX_DRY_RUN_CYCLES      (10000)

// _maxAccelIndex = 199 gives very smooth acceleration
enum {_maxAccelIndex = 15, _maxAccelEntries, _maxBisectionTrys = 20};

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
	  
    void reset(const unsigned int us) {
	    _totalClockTicks = 0;
#if CYGWIN
        //_cumulativeClockTicks = 0;
#endif /* CYGWIN */        
		accelMicroSec(us);
		_currentClockTicks = curveIndexToClockTicks(0);
	}
    
#if CYGWIN    
    void initCumulativeTime(void) {_cumulativeClockTicks = 0;}
#endif /* CYGWIN */
    
	// set/get acceleration time
    void accelMicroSec(const unsigned int us) {if (us > ACCEL_MAX_MICROSEC) {_accelMicroSec = ACCEL_MAX_MICROSEC;} if (us < ACCEL_MIN_MICROSEC) {_accelMicroSec = ACCEL_MIN_MICROSEC;} _accelMicroSec = us;}
    unsigned int accelMicroSec(void) const {return _accelMicroSec;}
	
    unsigned int dryRunAccel(void);
	void frequency(const unsigned int fmin = 200, const unsigned int fmax = 3200);
    // Convert RPM to frequency
    void RPMx10k(const unsigned int RPMx10kmin, const unsigned int RPMx10kmax) {frequency(_RPMx10ktoFreq(RPMx10kmin), _RPMx10ktoFreq(RPMx10kmax));}
    unsigned int fmin(void) const {return _fmin;}
    unsigned int fmax(void) const {return _fmax;}
	
	unsigned int curveIndexToClockTicks(const unsigned int index) {assert(index <= _maxAccelIndex); return _curveClockTicks[index];}
	unsigned int microSecToSteps(const unsigned int us);
	unsigned int stepsToMicroSec(const unsigned int steps);
	
    // Called from timer interrupt service routine
	unsigned int updateClockPeriod(void) {
		_totalClockTicks += _currentClockTicks;
#if CYGWIN
        // Used for debugging to get a profile of time vs frequency (speed)
        _cumulativeClockTicks += _currentClockTicks;
#endif /* CYGWIN */        
		unsigned int index = clockTicksToCurveIndex(_totalClockTicks);
#if ACCEL_LINEAR_FIT
        unsigned int moduloMicroSec = clockTicksToMicroSec(_totalClockTicks) % _accelMicroSecStep();
        float interpolateFreq = _linearInterpolate[index] * moduloMicroSec + _curveFreq[index];
        // TODO: Add mechanism so _currentClockTicks never varies from prior value by too much
        _currentClockTicks = freqToClockTicks(interpolateFreq);
#else // not ACCEL_LINEAR_FIT        
		/* 
		if (index >= _maxAccelEntries - 1) {
			// Can't get here.
			return 0xffffffff;
		}
		*/
		_currentClockTicks = curveIndexToClockTicks(index);
#endif // ACCEL_LINEAR_FIT        
		return _currentClockTicks;
	}
	// Used during deceleration where curve is traversed backwards (i.e. right to left)
	unsigned int updateClockPeriodDecelerate(void) {
		_totalClockTicks += _currentClockTicks;
#if CYGWIN
        _cumulativeClockTicks += _currentClockTicks;
#endif /* CYGWIN */        
		unsigned int index = clockTicksToCurveIndexDecelerate(_totalClockTicks);
		_currentClockTicks = curveIndexToClockTicks(index);
		return _currentClockTicks;
	}
	
#if CYGWIN	
    unsigned int updateCumulativeTimeWithClockPeriod(void) {
        _cumulativeClockTicks += _currentClockTicks;
        return _cumulativeClockTicks;
    }
#endif /* CYGWIN */    
    
	unsigned int clockTicksToCurveIndex(const unsigned int ct) const {
	    return microSecToCurveIndex(clockTicksToMicroSec(ct));
	}
	unsigned int clockTicksToCurveIndexDecelerate(const unsigned int ct) const {
	    return microSecToCurveIndexDecelerate(clockTicksToMicroSec(ct));
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
		//return (unsigned int)(ct / _clockMHz);    // original code
        // ((ct * 2) / _clockMHz) + 1 / 2   -> this is what's being done on the line below but using fast multiplication and division
        return (unsigned int)(((ct << 1)/*multiply-by-2*/ / _clockMHz) + 1) >> 1/*divide-by-2*/;    // Done to correct for rounding loss in integer arithmetic, so 123.5 returns 124
	}
    unsigned int microSecToClockTicks(const unsigned int us) const {
        return (unsigned int)(us * _clockMHz);
    }
	
    // Given microsec, return acceleration curve index which contains speed information
	unsigned int microSecToCurveIndex(const unsigned int us) const {
		if (us > accelMicroSec()) {
			return _maxAccelIndex;
		}
#if 0
		unsigned int index = us * _maxAccelIndex / accelMicroSec();
        unsigned int accelus = accelMicroSec();
		cout << "microSecToCurveIndex: us: " << us << " accelMicroSec: " << accelus << " index: " << index << endl;
#endif /* DUMP */
#if ACCEL_LINEAR_FIT
        return us / _accelMicroSecStep();
#else // not ACCEL_LINEAR_FIT        
		return (us * _maxAccelIndex) / accelMicroSec();
#endif // ACCEL_LINEAR_FIT        
	}
	unsigned int microSecToCurveIndexDecelerate(const unsigned int us) const {
		if (us > accelMicroSec()) {
			return 0;
		}
		return _maxAccelIndex - ((us * _maxAccelIndex) / accelMicroSec()) - 1;
	}

    void degreesPerMicrostepx10k(const unsigned int dpus) {_degreesPerMicrostepx10k = dpus;}
    unsigned int degreesPerMicrostepx10k(void) const  {return _degreesPerMicrostepx10k;}
    
    unsigned int totalClockTicks(void) const {return _totalClockTicks;}
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
        oss() << "_RPMx10ktoFreq rpmx10k: " << RPMx10k << " frequency: " << (RPMx10k * DEGREES_PER_REV_TIMES_MINUTE_PER_SEC) / _degreesPerMicrostepx10k;
        dump();
#endif /* REGRESS_1 */    
        return (RPMx10k * DEGREES_PER_REV_TIMES_MINUTE_PER_SEC) / _degreesPerMicrostepx10k;
    }
    
#if DUMP	
	void _dumpCurveFreq(void);
	void _dumpCurveClockTicks(void);
	void _dumpLinearInterpolate(void);
#endif /* DUMP */
#if ACCEL_LINEAR_FIT
    unsigned int _accelMicroSecStep(void) const {return accelMicroSec() / _maxAccelIndex;} // Equidistant steps (in microseconds) between frequency changes
#endif // ACCEL_LINEAR_FIT    
	
    float _curveFreq[_maxAccelEntries];
    unsigned int _curveClockTicks[_maxAccelEntries];
#if ACCEL_LINEAR_FIT    
    float _linearInterpolate[_maxAccelEntries];  // slope of frequency between points in _curveFreq[]
#endif // ACCEL_LINEAR_FIT    
    
    unsigned int _totalClockTicks;      // accumulated time in the acceleration profile
#if CYGWIN    
    unsigned int _cumulativeClockTicks; // accumulated time in the entire velocity profile
#endif /* CYGWIN */    
    unsigned int _currentClockTicks;    // period of current timer interrupt. Inversely proportional to motor speed.
    unsigned int _accelMicroSec;        // acceleration time in us
	unsigned int _fmin, _fmax;	        // frequency min/max for acceleration

    const float _clockMHz;
	const unsigned int _fStop;	        // frequency where we assume motor is as good as stopped
    unsigned int _degreesPerMicrostepx10k;
};

#endif /* _ACCEL_H_ */