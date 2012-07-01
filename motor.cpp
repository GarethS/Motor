/*
	Copyright (c) Gareth Scott 2011

	motor.cpp

    Test platform for stepper.cpp (located in /DriverLib/boards/ek-lm3s3748/motor)	

*/

#include "stepper.h"
//#include "log.h"

#include <iostream>
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
#endif /* CYGWIN */    
	//logc l("TESTa");
	//l.dump("start motor");
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
	
	//s.a.frequency(200, 1600);
    //s.a.time(500000);
	s.moveAbsolute(6000);

#if !CYGWIN
    for (;;) {
        if (s.state() == 0 /*IDLE*/) {
            s.moveRelative(-6000);
        }
    }
#endif /* CYGWIN */
	
	cout << "done" << endl;
	
	return 0;
}