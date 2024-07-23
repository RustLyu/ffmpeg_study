#ifndef VIDEO_STATE_H
#define VIDEO_STATE_H

#include "PacketQueue.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

struct Buffer
{
	Buffer()
	{
		buffer = nullptr;
		size = 0;
		read_index = 0;
	}
	uint8_t* buffer;
	int size;
	int read_index;
	std::mutex m;
	std::condition_variable cv;
};

struct StreamParam {
	PacketQueue<AVPacket> pkt_queue;
	AVCodecContext* codec_ctx;
	AVCodecParameters* param;
	AVCodec* codec;
	int index;
};

class VideoState {
public:
	VideoState();
	~VideoState();
public:
	int start();
	void push(AVPacket* pkt);
	AVPacket pop_audio();
	AVPacket pop_video();
	
	//void push_audio(uint8_t* f, int size);
	void push_video(AVFrame& f);

	const StreamParam& get_audio_param() {
		return audio_;
	}
	const StreamParam& get_video_param() {
		return video_;
	}

	const AVFrame& get_video_frame() {
		return video_buffer_.pop();
	}

	Buffer* get_audio_buffer() {
		return audio_buffer_;
	}

	void set_av_formate_ctx(AVFormatContext* ctx);


private:
	AVFormatContext* ctx_;
	StreamParam audio_;
	StreamParam video_;
	//PacketQueue<AVPacket> video_pkt_;
	//PacketQueue<AVPacket> audio_pkt_;

	//AVCodecContext* audio_ctx_;
	//AVCodecContext* video_ctx_;
	//AVCodecParameters* audio_param_;
	//AVCodecParameters* video_param_;
	//AVCodec* audio_codec_;
	//AVCodec* video_codec_;
	//int video_index_;
	//int audio_index_;
	Buffer* audio_buffer_;
	PacketQueue<AVFrame> video_buffer_;
};

#endif //VIDEO_STATE_H

