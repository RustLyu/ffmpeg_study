#include "demuxer.h"
#include "VideoState.h"
#include "common.h"

Demuxer::Demuxer(const char* url):
	url_(url), vs_(nullptr), stop_(false), ctx_(nullptr)
	, video_index_(-1),audio_index_(-1)
{
}

Demuxer::~Demuxer()
{
}

int Demuxer::start()
{
	if (url_.empty())
		return -1;
	if (ctx_ != nullptr)
		return -1;
	int ret = avformat_open_input(&ctx_, url_.c_str(), nullptr, nullptr);
	if (ret != 0)
		return -1;
	ret = avformat_find_stream_info(ctx_, nullptr);
	if (ret != 0)
		return -1;
	video_index_ = av_find_best_stream(ctx_, AVMediaType::AVMEDIA_TYPE_VIDEO, -1,-1, nullptr, 0);
	audio_index_ = av_find_best_stream(ctx_, AVMediaType::AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
	if (video_index_ == -1 && audio_index_ == -1)
		return -1;
	vs_->set_av_formate_ctx(ctx_);
	AVPacket pkt;
	th_ = std::thread([&]() {
		AVPacket pkt;
		while (av_read_frame(ctx_, &pkt) >= 0)
		{
			LOG("push to vs state");
			vs_->push(&pkt);
		}
	});
	th_.detach();
}

void Demuxer::stop()
{
	stop_ = true;
}

void Demuxer::set_video_state(VideoState* vs)
{
	vs_ = vs;
}
