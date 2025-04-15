#pragma once

#include <chrono>

//class CaltulateTime
//{
//public:
//	CaltulateTime(const char* desc);
//	~CaltulateTime();
//
//public:
//	void start();
//	void end();
//
//private:
//	std::chrono::steady_clock::time_point start_;
//	std::string desc_;
//};

#define TIMER_SCOPE(name) \
    auto _timer_ = [&]{ \
        auto _start = std::chrono::high_resolution_clock::now(); \
        return std::shared_ptr<void>(nullptr, [=](...) { \
            auto _end = std::chrono::high_resolution_clock::now(); \
            auto _dur = std::chrono::duration_cast<std::chrono::nanoseconds>(_end - _start).count(); \
            std::cout << "[TIMER] " << name << " took " << _dur << " ns\n"; \
        }); \
    }()
