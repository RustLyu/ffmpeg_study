#include "VideoDecoder.h"
#include "VideoState.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
}

VideoDecoder::VideoDecoder()
{
}

VideoDecoder::~VideoDecoder()
{
}

void VideoDecoder::start()
{
	sws_ = sws_getContext(vs_->get_video_param().codec_ctx->width, vs_->get_video_param().codec_ctx->height,
		vs_->get_video_param().codec_ctx->pix_fmt, vs_->get_video_param().codec_ctx->width, vs_->get_video_param().codec_ctx->height,
		AV_PIX_FMT_YUV420P, SWS_BILINEAR, nullptr, nullptr, nullptr);

	th_ = std::thread([&]() {
		while (1)
		{
			AVPacket pkt = vs_->pop_video();
			avcodec_send_packet(vs_->get_audio_param().codec_ctx, &pkt);
			AVFrame frame, frameYUV;

			while (auto ret = avcodec_receive_frame(vs_->get_video_param().codec_ctx, &frame) == 0) {
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
					break;
				}
				else if (ret < 0) {
					break;
				}
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
					break;
				else if (ret < 0)
					return -1;
				sws_scale(sws_, (uint8_t const* const*)frame.data, frame.linesize, 0,
					vs_->get_video_param().codec_ctx->height, frameYUV.data, frameYUV.linesize);
				vs_->push_video(frameYUV);
			}
			
		}});
	th_.detach();

}

void VideoDecoder::set_video_state(VideoState* vs)
{
	vs_ = vs;
}
