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
            AVPacket pkt = vs_->pop_audio_pkt();
            avcodec_send_packet(vs_->get_audio_param().codec_ctx, &pkt);
            while (auto ret = avcodec_receive_frame(vs_->get_audio_param().codec_ctx, frame) == 0) {
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
                //std::cout << "audio pts:" << frame->pts * av_q2d(vs_->get_audio_param().codec_ctx->time_base) << std::endl;
                int write_size = -1;
                do
                {
                    write_size = buf->write((char*)buffer, out_buffer_size);
                    //if (write_size < 0)
                    //    std::this_thread::sleep_for(std::chrono::microseconds(1));
                } 
                while (write_size < 0);
            }
        }});
	th_.detach();
}
