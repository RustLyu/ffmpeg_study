#pragma once

#include <chrono>

class CaltulateTime
{
public:
	CaltulateTime(const char* desc);
	~CaltulateTime();

public:
	void start();
	void end();

private:
	std::chrono::steady_clock::time_point start_;
	std::string desc_;
};
