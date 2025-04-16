#include "VideoDecoder.h"
#include "VideoState.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
}

VideoDecoder::VideoDecoder() : sws_(nullptr), vs_(nullptr), running_(false)
{
}

VideoDecoder::~VideoDecoder()
{
    stop();
    if (sws_) {
        sws_freeContext(sws_);
        sws_ = nullptr;
    }
}

void VideoDecoder::start()
{
    if (!vs_) return;

    std::lock_guard<std::mutex> lock(mutex_);
    if (running_) return;

    sws_ = sws_getContext(vs_->get_video_param().codec_ctx->width, vs_->get_video_param().codec_ctx->height,
        vs_->get_video_param().codec_ctx->pix_fmt, vs_->get_video_param().codec_ctx->width, vs_->get_video_param().codec_ctx->height,
        AV_PIX_FMT_YUV420P, SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!sws_) return;

    running_ = true;
    th_ = std::thread([this]() {
        decode_thread();
    });
}

void VideoDecoder::stop()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
    }
    if (th_.joinable()) {
        th_.join();
    }
}

void VideoDecoder::decode_thread()
{
    std::unique_ptr<AVFrame, void(*)(AVFrame*)> frame(av_frame_alloc(), [](AVFrame* f) { av_frame_free(&f); });
    std::unique_ptr<AVFrame, void(*)(AVFrame*)> frameYUV(av_frame_alloc(), [](AVFrame* f) { av_frame_free(&f); });
    std::unique_ptr<uint8_t, void(*)(void*)> buffer(nullptr, av_free);

    if (!frame || !frameYUV) return;

    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, vs_->get_video_param().codec_ctx->width,
        vs_->get_video_param().codec_ctx->height, 32);
    buffer.reset((uint8_t*)av_malloc(num_bytes * sizeof(uint8_t)));
    if (!buffer) return;

    av_image_fill_arrays(frameYUV->data, frameYUV->linesize, buffer.get(), AV_PIX_FMT_YUV420P,
        vs_->get_video_param().codec_ctx->width, vs_->get_video_param().codec_ctx->height, 32);

    while (running_)
    {
        AVPacket pkt = vs_->pop_video();
        if (avcodec_send_packet(vs_->get_video_param().codec_ctx, &pkt) < 0) {
            continue;
        }

        int ret;
        while ((ret = avcodec_receive_frame(vs_->get_video_param().codec_ctx, frame.get())) >= 0) {
            sws_scale(sws_, (uint8_t const* const*)frame->data, frame->linesize, 0,
                vs_->get_video_param().codec_ctx->height, frameYUV->data, frameYUV->linesize);
            vs_->push_video(*frameYUV);
        }

        if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
            // Handle error
            break;
        }
    }
}

void VideoDecoder::set_video_state(VideoState* vs)
{
    std::lock_guard<std::mutex> lock(mutex_);
    vs_ = vs;
}
