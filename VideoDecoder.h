#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H

#include <thread>

struct SwsContext;
class VideoState;

class VideoDecoder {
public:
	VideoDecoder();
	~VideoDecoder();

public:
	void start();
	void set_video_state(VideoState* vs);
private:
	SwsContext* sws_;
	std::thread th_;
	VideoState* vs_;
};

#endif // VIDEO_DECODER_H
