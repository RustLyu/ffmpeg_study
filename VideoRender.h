#ifndef VIDEO_RENDER_H
#define VIDEO_RENDER_H

#include <thread>
#include <condition_variable>
#include <mutex>
#include <SDL.h>

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
	void stop();

private:
	void cleanup();

	VideoState* vs_;
	SwsContext* sws_;
	std::thread th_;
	bool running_ = false;
	SDL_Window* win_ = nullptr;
	SDL_Renderer* renderer_ = nullptr;
	SDL_Texture* texture_ = nullptr;
	std::mutex mutex_;
	bool fullscreen_ = false;
};

#endif  //VIDEO_RENDER_H
