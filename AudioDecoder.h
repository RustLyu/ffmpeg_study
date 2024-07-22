#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H

#include <thread>
#include <condition_variable>

#include "PacketQueue.h"

class VideoState;
struct SwrContext;

class AudioDecoder {
public:
	AudioDecoder();
	~AudioDecoder();

public:
	void set_video_state(VideoState* vs);
	void start();

private:
	VideoState* vs_;
	SwrContext* swr_;
	std::thread th_;
};

#endif  //AUDIO_DECODER_H
