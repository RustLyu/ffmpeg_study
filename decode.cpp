#include <iostream>
#include <string>

#include "common.h"
#include "decode.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/error.h>
}

AVFormatContext* FFmpegStudy::open_file(const char* path) {
    AVFormatContext* ret = nullptr;

    if (avformat_open_input(&ret, path, nullptr, nullptr))
        return ret;

    if (avformat_find_stream_info(ret, nullptr) < 0)
        return ret;
    av_dump_format(ret, 0, path, 0);
    return ret;
}

std::tuple<int, int> FFmpegStudy::get_audio_vedio_index(AVFormatContext* ctx)
{
    int video_index = -1;
    int audio_index = -1;
    for (int i = 0; i < ctx->nb_streams; ++i)
    {
        if (ctx->streams[i]->codecpar->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO)
        {
            audio_index = i;
        }
        else if (ctx->streams[i]->codecpar->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO)
        {
            video_index = i;
        }
    }
    return { audio_index, video_index };
}


void FFmpegStudy::close(AVFormatContext* ctx)
{
    avformat_close_input(&ctx);
}

int FFmpegStudy::save_fame_as_jpeg(AVFrame* frame, AVCodecContext* ctx, int no)
{
    AVCodec* jpeg_codec = const_cast<AVCodec*>(avcodec_find_encoder(AV_CODEC_ID_MJPEG));
    AVCodecContext* jpeg_ctx = avcodec_alloc_context3(jpeg_codec);
    jpeg_ctx->pix_fmt = AV_PIX_FMT_YUVJ420P;
    jpeg_ctx->height = ctx->height;
    jpeg_ctx->width = ctx->width;
    jpeg_ctx->time_base = AVRational(1, 25);
    if (avcodec_open2(jpeg_ctx, jpeg_codec, nullptr) < 0)
        return -1;
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = nullptr;
    pkt.size = 0;

    auto ret = avcodec_send_frame(jpeg_ctx, frame);
    while (ret >= 0)
    {
        ret = avcodec_receive_packet(jpeg_ctx, &pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR(AVERROR_EOF))
            return 0;
        else if (ret < 0)
        {
            std::cout << "errror" << std::endl;
        }
        //ret = av_write_frame
        char filename[1024];
        snprintf(filename, sizeof(filename), "frame-%d.jpg", no);
        FILE* f = fopen(filename, "wb");
        fwrite(pkt.data, 1, pkt.size, f);
        fclose(f);
        av_packet_unref(&pkt);
    }
}

int FFmpegStudy::decode(AVFormatContext* ctx, int video_index)
{
    const AVCodec* decoder = avcodec_find_decoder(ctx->streams[video_index]->codecpar->codec_id);
    if (decoder == nullptr)
        return -1;
    AVCodecContext* codec_ctx = avcodec_alloc_context3(decoder);
    if (int ret = avcodec_parameters_to_context(codec_ctx, ctx->streams[video_index]->codecpar) != 0)
        return -1;
    if (int ret = avcodec_open2(codec_ctx, decoder, nullptr) != 0)
        return -1;

    AVFrame* frame = av_frame_alloc();
    AVPacket pkt;
    
    av_init_packet(&pkt);
    pkt.data = nullptr;
    pkt.size = 0;

    int frame_no = 0;
    while (av_read_frame(ctx, &pkt) >= 0)
    {
        if (pkt.stream_index == video_index)
        {
            int ret = avcodec_send_packet(codec_ctx, &pkt);
            if (ret < 0)
            {
                ERRROR_CODE_2_STR(ret);
                std::cout << "error send pkt" << std::endl;
                break;
            }
            while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
                save_fame_as_jpeg(frame, codec_ctx, frame_no++);
            }
        }
        av_packet_unref(&pkt);
    }
    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
}
