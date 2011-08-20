/*
	Copyright (c) Gareth Scott 2011

	log.h

*/

#ifndef _LOG_H_
#define _LOG_H_

#include <iostream>
#include <fstream>

class logc {
public:
	logc(std::string n) {_myName = n;}
	~logc() {}

	void dump(const std::string& s);
	void dumpNoTimeStamp(const std::string& s);

private:
	std::string _timeStampPrefix(void);

	static std::ofstream _os;
	std::string _myName;
};

#endif /* _LOG_H_ */