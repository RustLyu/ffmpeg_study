#ifndef VIDEO_STATE_H
#define VIDEO_STATE_H

#include "PacketQueue.h"
#include <memory>
#include <mutex>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}
#include "RingBuffer.h"

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

	// Prevent copying
	VideoState(const VideoState&) = delete;
	VideoState& operator=(const VideoState&) = delete;

public:
	int start();
	void push(AVPacket* pkt);
	AVPacket pop_audio_pkt();
	AVPacket pop_video_pkt();
	
	//void push_audio(uint8_t* f, int size);
	void push_video(AVFrame& f);

	const StreamParam& get_audio_param() {
		return audio_;
	}
	const StreamParam& get_video_param() {
		return video_;
	}

	const AVFrame& get_video_frame();

	RingBuffer* get_audio_buffer() {
		return audio_buffer_;
	}

	void set_av_formate_ctx(AVFormatContext* ctx);
	void set_audio_played_samples(uint64_t n)
	{
		audio_played_samples_ = n;
	}
	uint64_t get_audio_played_samples()
	{
		return audio_played_samples_;
	}
	void add_audio_played_samples(uint64_t n)
	{
		audio_played_samples_ += n;
	}
	double get_audio_clock() 
	{
		return (double)audio_played_samples_ / (audio_.codec_ctx->sample_rate * audio_.codec_ctx->ch_layout.nb_channels);
	}

private:
	AVFormatContext* ctx_;
	StreamParam audio_;
	StreamParam video_;
	RingBuffer* audio_buffer_;
	PacketQueue<AVFrame> video_buffer_;
	uint64_t audio_played_samples_;
};

#endif //VIDEO_STATE_H

