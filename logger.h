#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <thread>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <filesystem>

namespace FFmpeg_Test {

    class Logger {
    public:
        template<typename... Args>
        static void log(const char* file, int line, int pid, std::thread::id threadId, Args... args) {
            std::ostringstream oss;
            oss << "[" << current_tm() << "] ";
            oss << "[pid " << pid << ":";
            oss << " Thread " << threadId << "] ";
            oss << path_2_name(file) << ":" << line << " ";
            (oss << ... << args) << std::endl;
            std::cout << oss.str() << std::flush;
        }

    private:
        static std::string current_tm() {
            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);

            auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
            auto value = now_ms.time_since_epoch();
            auto millis = value.count() % 1000;
            std::ostringstream ss;
            ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X")
                << '.' << std::setw(3) << std::setfill('0') << millis;
            return ss.str();
        }

        static std::string path_2_name(const std::string& filePath) {
            return std::filesystem::path(filePath).filename().string();
        }
    };

}
#endif // LOGGER_H
