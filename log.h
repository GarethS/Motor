/*
	Copyright (c) Gareth Scott 2011

	log.h

*/

#ifndef _LOG_H_
#define _LOG_H_

#include <iostream>
#include <fstream>
#include <sstream>

class logc {
public:
	logc(std::string n) {_myName = n;}
	~logc() {}

	void dump(const std::string& s, const bool useTimeStamp = true);
	void dump(void) {dump(oss().str(), false);}
	std::ostringstream& oss(void) {return _oss;}
	void ossClear(void) {_oss.str("");}

private:
	std::string _timeStampPrefix(void);

	static std::ofstream _os;
	static std::ostringstream _oss;
	std::string _myName;
};

#endif /* _LOG_H_ */