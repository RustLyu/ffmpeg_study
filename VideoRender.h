#ifndef VIDEO_RENDER_H
#define VIDEO_RENDER_H

#include <thread>
#include <condition_variable>

#include "PacketQueue.h"

class VideoState;
struct SwsContext;

class VideoRender {
public:
	VideoRender();
	~VideoRender();

public:
	void set_video_state(VideoState* vs);
	void start();

private:
	VideoState* vs_;
	SwsContext* sws_;
	std::thread th_;
};

#endif  //VIDEO_RENDER_H
