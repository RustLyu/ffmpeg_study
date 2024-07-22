#ifndef COMMON_H
#define COMMON_H

#include "logger.h"

#define LOG(...) FFmpeg_Test::Logger::log(__FILE__, __LINE__, getpid(), std::this_thread::get_id(), __VA_ARGS__)

#define ERRROR_CODE_2_STR(code)\
    char buffer[1024];\
    memset(buffer, 0, 1024);\
    av_strerror(code, buffer, 1024);\
    fprintf(stderr, "error:'%s'\n", buffer)

#endif // COMMON_H
