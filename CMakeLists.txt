cmake_minimum_required(VERSION 3.10)
project(FFmpegExample)

set(CMAKE_CXX_STANDARD 11)

# 设置 FFmpeg 的 include 和 lib 路径
include_directories(/ucrt64/include)
link_directories(/ucrt64/lib)

# 添加可执行文件
add_executable(FFmpegExample main.cpp)

# 链接 FFmpeg 库
target_link_libraries(FFmpegExample avformat avcodec avutil)

