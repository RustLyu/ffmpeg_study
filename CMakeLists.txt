cmake_minimum_required(VERSION 3.10)
project(MyFFmpegProject)

set(CMAKE_CXX_STANDARD 23)

set(FFMPEG_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/../ffmpeg-master-latest-win64-gpl-shared/include)
set(FFMPEG_LIB_DIR ${CMAKE_SOURCE_DIR}/../ffmpeg-master-latest-win64-gpl-shared/lib)

set(SDL_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/../SDL2-2.30.5/include)
set(SDL_LIB_DIR ${CMAKE_SOURCE_DIR}/../SDL2-2.30.5/lib/x64/)

include_directories(${FFMPEG_INCLUDE_DIR} ${SDL_INCLUDE_DIR})

link_directories(${FFMPEG_LIB_DIR} ${SDL_LIB_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")

add_executable(MyFFmpegProject main.cpp
	common.h
	decode.h decode.cpp
    encode.h encode.cpp 
	player.h player.cpp  
	playaudio.h playaudio.cpp
	PacketQueue.h demuxer.h
	demuxer.cpp VideoState.h
	VideoState.cpp
	RingBuffer.h
	splite.h splite.cpp 
	"AudioDecoder.cpp" "AudioDecoder.h" 
	"render.h" "render.cpp" 
	"VideoDecoder.cpp" "VideoDecoder.h" 
	"AudioRender.cpp" "AudioRender.h" 
	"CaltulateTime.cpp" "CaltulateTime.h" 
	"logger.h"
	"VideoRender.h" "VideoRender.cpp"
	)

target_link_libraries(MyFFmpegProject avcodec avformat avutil swscale SDL2 swresample)
