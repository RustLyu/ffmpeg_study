#include "AudioDecoder.h"
#include "VideoState.h"
extern "C" {
#include "libswresample/swresample.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
#include "CaltulateTime.h"

#include <iostream>

AudioDecoder::AudioDecoder()
{
}

AudioDecoder::~AudioDecoder()
{
}

void AudioDecoder::set_video_state(VideoState* vs)
{
	vs_ = vs;
}

void AudioDecoder::start()
{
    swr_ = swr_alloc();
    swr_alloc_set_opts2(&swr_, &vs_->get_audio_param().codec_ctx->ch_layout, AV_SAMPLE_FMT_S16,
        vs_->get_audio_param().codec_ctx->sample_rate, &vs_->get_audio_param().codec_ctx->ch_layout, 
        vs_->get_audio_param().codec_ctx->sample_fmt,
        vs_->get_audio_param().codec_ctx->sample_rate, 0, nullptr);

    swr_init(swr_);
    uint8_t* buffer = (uint8_t*)av_malloc(192000);
    AVFrame* frame = av_frame_alloc();
    th_ = std::thread([&, buffer, frame]() {
        while (1)
        {
            auto buf = vs_->get_audio_buffer();
            std::unique_lock<std::mutex> lock(buf->m);
            if (buf->size != buf->read_index)
                buf->cv.wait(lock, [&]() {
                return buf->size == buf->read_index;
                    });
            if (buf->buffer)
            {
                //free(buf->buffer);
                //buf->buffer = nullptr;
                buf->size = 0;
                buf->read_index = 0;
            }
            AVPacket pkt = vs_->pop_audio();
            //std::cout << "pop from vs state" << std::endl;
            avcodec_send_packet(vs_->get_audio_param().codec_ctx, &pkt);
            while (auto ret = avcodec_receive_frame(vs_->get_audio_param().codec_ctx, frame) == 0) {
                CaltulateTime cal("pop audio");
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                }
                else if (ret < 0) {
                    break;
                }
                int dst_nb_samples = swr_get_out_samples(swr_, frame->nb_samples);
                int out_buffer_size = av_samples_get_buffer_size(NULL, vs_->get_audio_param().codec_ctx->ch_layout.nb_channels,
                    dst_nb_samples, AV_SAMPLE_FMT_S16, 1);
                memset(buffer, 0 , 192000);
                swr_convert(swr_, &buffer, out_buffer_size,
                    (const uint8_t**)frame->data, frame->nb_samples);
                auto size = out_buffer_size;
                buf->buffer = buffer;
                buf->size = out_buffer_size;
                buf->read_index = 0;
                cal.end();
            }
        }});
	th_.detach();
}
