#include "CaltulateTime.h"
#include "common.h"

#include <iostream>

CaltulateTime::CaltulateTime(const char* desc)
{
	desc_ = desc;
	start();
}

CaltulateTime::~CaltulateTime()
{
}

void CaltulateTime::start()
{
	start_ = std::chrono::high_resolution_clock::now();
}

void CaltulateTime::end()
{
	auto end = std::chrono::high_resolution_clock::now();
	//std::cout << desc_ << " : " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start_) << std::endl;
	LOG(desc_.c_str(), " ", std::chrono::duration_cast<std::chrono::milliseconds>(end - start_));
	start_ = end;
}
