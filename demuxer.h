#ifndef DEMUXER_H
#define DEMUXER_H

#include <iostream>
#include <thread>

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}
class VideoState;
class Demuxer {

public:
	Demuxer(const char* url);
	~Demuxer();

public:
	int start();
	void stop();
	void set_video_state(VideoState* vs);

private:
	AVFormatContext* ctx_;
	std::string url_;
	std::thread th_;
	VideoState* vs_;
	int video_index_;
	int audio_index_;
	bool stop_;
};

#endif //DEMUXER_H
