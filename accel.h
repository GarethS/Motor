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
#define MICROSEC_PER_SEC        (1000000)
#define DEGREES_PER_REV         (360)
#define DEGREES_PER_REV_TIMES_MINUTE_PER_SEC    (6) // 360 * 0.01666 = 6
#define DEGREES_PER_STEP_X10000 (1125)  // See comment for _degPerStepX10000 below.
#define DEGREES_PER_MICROSTEP_NOMINAL   (1.8)   // A lot of stepper motors are 1.8 degrees per full-step
//#define STEP_PER_DEGREE         (16.0 / 1.8)    // Assuming 16-microsteps, 8.889 
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
#if CYGWIN
        //_cumulativeClockTicks = 0;
#endif /* CYGWIN */        
		time(t);
		_currentClockTicks = curveIndexToClockTicks(0);
	}
    
#if CYGWIN    
    void initCumulativeTime(void) {_cumulativeClockTicks = 0;}
#endif /* CYGWIN */
    
	// set/get acceleration time
    void time(const unsigned int us) {_time = us;}
    unsigned int time(void) {return _time;}
	
    unsigned int dryRunAccel(void);
	void frequency(const unsigned int fmin = 200, const unsigned int fmax = 1200);
    // Convert RPM to frequency
    void RPMx10k(const unsigned int RPMx10kmin, const unsigned int RPMx10kmax) {frequency(_RPMx10ktoFreq(RPMx10kmin), _RPMx10ktoFreq(RPMx10kmax));}
    void RPM(const float RPMmin, const float RPMmax) {frequency(_RPMtoFreq(RPMmin), _RPMtoFreq(RPMmax));}
    unsigned int fmin(void) {return _fmin;}
    unsigned int fmax(void) {return _fmax;}
	
    //int acceleration(void) {/* TODO */}
    //int velocity(void) {/* TODO */}

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
	
	unsigned int clockTicksToCurveIndex(unsigned int ct) {
	    return microSecToCurveIndex(clockTicksToMicroSec(ct));
	}
	unsigned int clockTicksToCurveIndexReverse(unsigned int ct) {
	    return microSecToCurveIndexReverse(clockTicksToMicroSec(ct));
	}

	bool freqCloseToStop(unsigned int f) {
		if (f <= _fStop) {
			return true;
		}
		return false;
	}
	
	// Given clock ticks, return frequency
	unsigned int freqFromClockTicks(unsigned int ct) {
		if (clockTicksToMicroSec(ct) == 0) {
			// Avoid divide-by-zero
			return INT_MAX;
		}
		return _microSecPerSec / clockTicksToMicroSec(ct);
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
		return us * _maxAccelIndex / time();
	}
	// Same as microSecToCurveIndex(), except returns curve index in reverse order. 
	//  Used for deceleration.
	unsigned int microSecToCurveIndexReverse(const unsigned int us) {
		if (us > time()) {
			return 0;
		}
		return _maxAccelIndex - (us * _maxAccelEntries / time());
	}

	// Given clock ticks, return equivalent microsec.
    unsigned int clockTicksToMicroSec(const unsigned int ct) {
		return (unsigned int)((ct / _clockMHz));
	}
	
    void degreesPerMicrostep(float dpus) {_degreesPerMicrostep = dpus;}
    float degreesPerMicrostep(void) const  {return _degreesPerMicrostep;}
    
#if CYGWIN    
    unsigned int currentClockTicks(void) const {return _currentClockTicks;}
    unsigned int cumulativeClockTicks(void) const {return _cumulativeClockTicks;}
#endif /* CYGWIN */    
    
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
    
    unsigned int _RPMx10ktoFreq(unsigned int RPMx10k) {
#if REGRESS_1
        oss() << "_RPMx10ktoFreq rpmx10k:" << RPMx10k << " frequency:" << (unsigned int)(RPMx10k * DEGREES_PER_REV_TIMES_MINUTE_PER_SEC / _degreesPerMicrostep / 10000.0);
        dump();
#endif /* REGRESS_1 */    
        return (unsigned int)(RPMx10k * DEGREES_PER_REV_TIMES_MINUTE_PER_SEC / _degreesPerMicrostep / 10000.0);
    }
    
    unsigned int _RPMtoFreq(float rpm) {
#if REGRESS_1
        oss() << "_RPMtoFreq rpm:" << rpm << " frequency:" << (unsigned int)(rpm * DEGREES_PER_REV_TIMES_MINUTE_PER_SEC / _degreesPerMicrostep);
        dump();
#endif /* REGRESS_1 */    
        return (unsigned int)(rpm * DEGREES_PER_REV_TIMES_MINUTE_PER_SEC / _degreesPerMicrostep);
    }
    
#if DUMP	
	void _dumpCurveFloat(void);
	void _dumpCurveInt(void);
#endif /* DUMP */
	
    //enum {_maxAccelIndex = 99, _maxAccelEntries, _maxBisectionTrys = 20};
    float _curveFloat[_maxAccelEntries];
    unsigned int _curveInt[_maxAccelEntries];
    
    unsigned int _totalClockTicks;      // accumulated time in the acceleration profile
#if CYGWIN    
    unsigned int _cumulativeClockTicks; // accumulated time in the entire velocity profile
#endif /* CYGWIN */    
    unsigned int _currentClockTicks;    // period of current timer interrupt. Inversely proportional to motor speed.
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
    // 0 microsteps = 1.8 deg/microstep -> 0.556 ustep/deg
    // 2 microsteps = 1.8/2 = 0.9       -> 1.111 ustep/deg
    // 4 microsteps = .45               -> 2.222 ustep/deg
    // 8 microsteps = .225              -> 4.444 ustep/deg
    // 16 microsteps = .1125
    // 32 microsteps = .05625
    float _degreesPerMicrostep;
};

#endif /* _ACCEL_H_ */