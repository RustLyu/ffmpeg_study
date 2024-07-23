#include "VideoState.h"

//extern "C" {
////#include "libswresample/swresample.h"
//#include <libavcodec/avcodec.h>
//#include <libavformat/avformat.h>
////#include <libswscale/swscale.h>
//}

VideoState::VideoState()
{
	audio_buffer_ = new Buffer();
}

VideoState::~VideoState()
{
}

int VideoState::start()
{
	int ret = -1;
	if (ctx_ == nullptr)
		return -1;
	video_.index = av_find_best_stream(ctx_, AVMediaType::AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
	audio_.index = av_find_best_stream(ctx_, AVMediaType::AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
	if (audio_.index == -1 && video_.index == -1)
		return -1;
	if (audio_.index >= -1)
	{
		audio_.param = ctx_->streams[audio_.index]->codecpar;
		audio_.codec = const_cast<AVCodec*>(avcodec_find_decoder(audio_.param->codec_id));
		audio_.codec_ctx = avcodec_alloc_context3(audio_.codec);
		avcodec_parameters_to_context(audio_.codec_ctx, audio_.param);
		ret = avcodec_open2(audio_.codec_ctx, audio_.codec, nullptr);
		if (ret != 0)
			return -1;
	}

	if (video_.index >= 0)
	{
		video_.param = ctx_->streams[video_.index]->codecpar;
		video_.codec = const_cast<AVCodec*>(avcodec_find_decoder(video_.param->codec_id));
		video_.codec_ctx = avcodec_alloc_context3(video_.codec);
		avcodec_parameters_to_context(video_.codec_ctx, video_.param);
		ret = avcodec_open2(video_.codec_ctx, video_.codec, nullptr);
		if (ret != 0)
			return -1;
	}

	return 0;
}

void VideoState::push(AVPacket* pkt)
{
	if (pkt->stream_index == video_.index)
	{
		video_.pkt_queue.push(*pkt);
	}
	else if (pkt->stream_index == audio_.index)
	{
		audio_.pkt_queue.push(*pkt);
	}

}

AVPacket VideoState::pop_audio()
{
	return audio_.pkt_queue.pop();
}

AVPacket VideoState::pop_video()
{
	return video_.pkt_queue.pop();
}

//void VideoState::push_audio(uint8_t* f, int size)
//{
//	audio_buffer_.buffer = f;
//	audio_buffer_.size = size;
//}

void VideoState::push_video(AVFrame& f)
{
	video_buffer_.push(f);
}

void VideoState::set_av_formate_ctx(AVFormatContext* ctx)
{
	ctx_ = ctx;
}
