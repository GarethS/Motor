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
	s.dryRunAccel();
	
	cout << "done" << endl;
	
	return 0;
}