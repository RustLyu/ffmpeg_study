#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H

#include <thread>
#include <mutex>

struct SwsContext;
class VideoState;

class VideoDecoder {
public:
	VideoDecoder();
	~VideoDecoder();

public:
	void start();
	void stop();
	void set_video_state(VideoState* vs);

private:
	void decode_thread();

private:
	SwsContext* sws_;
	std::thread th_;
	VideoState* vs_;
	std::mutex mutex_;
	bool running_;
};

#endif // VIDEO_DECODER_H
