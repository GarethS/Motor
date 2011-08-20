/*
	Copyright (c) Gareth Scott 2011

	motor.cpp

    Test platform for stepper.cpp (located in /DriverLib/boards/ek-lm3s3748/motor)	

*/

#include "../../../../DriverLib/boards/ek-lm3s3748/motor/stepper.h"
//#include "log.h"

#include <iostream>
using namespace std;

int main(void) {
	//logc l("TESTa");
	//l.dump("start motor");
	stepper s; 
	//s.dryRunAccel();
	//cout << "acceleration steps= " << s._accelGetStepCount() << endl;
	s.a._bisectTimeForStepCount(1000);
	s.a._bisectTimeForStepCount(500);
	s.a._bisectTimeForStepCount(999);
	s.a._bisectTimeForStepCount(10);
	
	cout << "done" << endl;
	
	return 0;
}