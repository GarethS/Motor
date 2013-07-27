/*
	Copyright (c) Gareth Scott 2011

	motor.cpp

    Test platform for stepper.cpp (located in /DriverLib/boards/ek-lm3s3748/motor)	

*/

#include "stepper.h"
#include "led.h"
//#include "log.h"

#include <iostream>
#include <vector>
#if CYGWIN
using namespace std;
#endif /* CYGWIN */

#if CYGWIN
int main(void) {
#else /* not CYGWIN */
stepper* sp = NULL;

extern "C" void stepperISR(void) {
    if (sp != NULL) {
        sp->isr();
    }
}

extern "C" int mainA(void); 

int mainA(void) {
    led::enable(1);    
#endif /* CYGWIN */    
	//logc l("TESTa");
	//l.dump("start motor");
#if 0 
    // Possibly used for a test. Not sure.
    std::vector<int> abc;
    abc.push_back(1); // [0]
    abc.push_back(21); // [1]
    
    cout << abc[0] << " " << abc[1] << endl;
    return 1;
#endif    
    
	stepper s;
#if !CYGWIN
    sp = &s;
#endif /* not CYGWIN */    
#if 0
    // Turn on to get log of frequency vs cumulative time
    s.a.time(2000000);
	s.a.dryRunAccel();
    s.a.time(1000000);  // Set back to default acceleration time
#endif    
	//cout << "acceleration steps= " << s._accelGetStepCount() << endl;
#if REGRESS_1	
	s.a.stepsToTime(1000);
	s.a.stepsToTime(500);
	s.a.stepsToTime(999);
	s.a.stepsToTime(10);
	s.a.test();
	
	s.test();
#endif /* REGRESS_1 */
	
	//s.a.frequency(200, 1650); // Fastest for x2 usteps
	//s.a.frequency(200, 406);    // 1 rev/sec for x2 usteps
	//s.a.frequency(200, 1624);   // 1 rev/sec for x8 usteps
    
    s.degreesPerMicrostep(MICROSTEPS_8_STD);
    //s.RPMx10k(8 * 10000, 120 * 10000);  // 8 rpm start and 120 rpm top speed
    s.RPM(8.0, 120.0);
	//s.frequency(200, 6600);   // Fastest for x8 usteps
    s.accelerationTimeMicrosecs(500000);
    s.positionSteps(0);
	s.moveAbsoluteDegree(360);
	//s.moveAbsolute(16000);
    //s.runVirtualMotor();

#if !CYGWIN
    int direction = -1;
    int counter = 0;
    for (;;) {
        if (s.state() == 0 /*IDLE*/) {
#if 0         
            s.velocity(2800);
            //s.moveRelative(1600 * direction);
#else       
            if (counter++ > 10) {
                //break;
            }
            if (direction == -1) {
      	        s.moveAbsoluteDegree(0);
            } else {
      	        s.moveAbsoluteDegree(720);
            }
#endif            
            direction *= -1;
        }
    }
#else /* CYGWIN */    
	cout << "done" << endl;
#endif /* CYGWIN */
	return 0;
}