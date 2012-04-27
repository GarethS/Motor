/*
	Copyright (c) Gareth Scott 2011

	log.cpp

*/

#include <sstream>
#include <iomanip>	// for setw(), setfill() etc.
#include <ctime>

#include "log.h"

std::ofstream logc::_os("log.txt", std::ios::app);	// Makes class look like a singleton by making this static
std::ostringstream logc::_oss;

void logc::dump(const std::string& s, const bool useTimeStamp /* = true */, const bool toScreen /* = true */) {
	if (useTimeStamp) {
		std::string ts = _timeStampPrefix() + _myName + " " + s;
		if (toScreen) {
			std::cout << ts << std::endl;	// Output to screen
		}
		if (_os != NULL) {
			_os << ts << std::endl;	// Output to file if it exists
		}
	} else {
        std::string ts = _myName + " " + s;
		if (toScreen) {
			std::cout << ts << std::endl;	// Output to screen
		}
		if (_os != NULL) {
			_os << ts << std::endl;	// Output to file if it exists
		}
	}
	ossClear();
}

// Typical output: '23/02/11 12:36:09'
std::string logc::_timeStampPrefix(void) {
	time_t rawTime;
	time(&rawTime);
	struct tm* t;
	t = localtime(&rawTime);
	//localtime_s(&t, &rawTime);
#if 1
	char buf[64];
	strftime(buf, 64, "%a %d/%b/%y %H:%M:%S ", t);
	std::string s(buf);
	return s;
#else
	std::ostringstream oss;
	oss << std::setw(2) << std::setfill('0') << t.tm_mday << "/"
		<< std::setw(2) << std::setfill('0') << t.tm_mon+1 << "/"
		<< std::setw(2) << std::setfill('0') << t.tm_year-100 /*year since 1900, so -100*/ << " "
		<< std::setw(2) << std::setfill('0') << t.tm_hour << ":"
		<< std::setw(2) << std::setfill('0') << t.tm_min << ":"
		<< std::setw(2) << std::setfill('0') << t.tm_sec << " ";
	return oss.str();
#endif
}
