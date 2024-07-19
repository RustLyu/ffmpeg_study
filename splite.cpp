#include <iostream>
#include <string>
#include "common.h"
#include "splite.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

void FFmpegStudy::splite_audio_video(const std::string& filename) {
    avformat_network_init();
    //av_log(NULL, AV_LOG_INFO, "Cannot open video decoder\n");
    //char buf[1024];
    //memset(buf, 0, 1024);
    AVFormatContext* formatContext = nullptr;
    auto open_ret = avformat_open_input(&formatContext, filename.c_str(), nullptr, nullptr);
    if (open_ret < 0) {
        ERRROR_CODE_2_STR(open_ret);
        //av_strerror(open_ret, buf, 1024);
        //fprintf(stderr, "Could not open input file '%s' reason:'%s'\n", filename.c_str(), buf);
        return;
    }

    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        std::cerr << "Could not retrieve stream info from file: " << filename << std::endl;
        avformat_close_input(&formatContext);
        return;
    }
    int audio_index = -1;
    int video_index = -1;
    for (auto i = 0; i < formatContext->nb_streams; ++i)
    {
        if (formatContext->streams[i]->codecpar->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO)
        {
            audio_index = i;
        }
        else if (formatContext->streams[i]->codecpar->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO)
        {
            video_index = i;
        }
    }
    if (audio_index == -1 || video_index == -1)
    {
        std::cout << "index error audio:" << audio_index << " v_index:" << video_index << std::endl;
        return;
    }
    std::string audio_file_path = "./audio.aac";
    std::string video_file_path = "./video.mp4";

    AVFormatContext* audio_ctx = nullptr;
    avformat_alloc_output_context2(&audio_ctx, nullptr, nullptr, audio_file_path.c_str());


    AVFormatContext* video_ctx = nullptr;
    avformat_alloc_output_context2(&video_ctx, nullptr, nullptr, video_file_path.c_str());

    if (audio_ctx == nullptr || video_ctx == nullptr)
    {
        std::cout << "alloc ctx error" << std::endl;
        return;
    }

    AVStream* audio_stream = avformat_new_stream(audio_ctx, nullptr);
    AVStream* video_stream = avformat_new_stream(video_ctx, nullptr);


    if (audio_stream == nullptr || video_stream == nullptr)
    {
        std::cout << "alloc strean ctx error" << std::endl;
        return;
    }

    avcodec_parameters_copy(audio_stream->codecpar, formatContext->streams[audio_index]->codecpar);
    avcodec_parameters_copy(video_stream->codecpar, formatContext->streams[video_index]->codecpar);


    if (!(audio_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&audio_ctx->pb, audio_file_path.c_str(), AVIO_FLAG_WRITE) < 0)
        {
            return;
        }
    }

    if (!(video_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&video_ctx->pb, video_file_path.c_str(), AVIO_FLAG_WRITE) < 0)
        {
            return;
        }
    }

    avformat_write_header(audio_ctx, nullptr);
    avformat_write_header(video_ctx, nullptr);

    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = nullptr;
    pkt.size = 0;


    while (av_read_frame(formatContext, &pkt) >= 0)
    {
        std::cout << "src:" << pkt.stream_index << " audio:" << audio_index << " video:" << video_index << std::endl;
        if (pkt.stream_index == video_index)
        {
            pkt.stream_index = video_stream->index;
            av_interleaved_write_frame(video_ctx, &pkt);
        }
        else if (pkt.stream_index == audio_index)
        {
            pkt.stream_index = audio_stream->index;
            av_interleaved_write_frame(audio_ctx, &pkt);
        }
    }
    av_write_trailer(audio_ctx);
    av_write_trailer(video_ctx);

    if (!(audio_ctx->oformat->flags & AVFMT_NOFILE))
    {
        avio_closep(&audio_ctx->pb);
    }

    if (!(video_ctx->oformat->flags & AVFMT_NOFILE))
    {
        avio_closep(&video_ctx->pb);
    }

    av_dump_format(formatContext, 0, filename.c_str(), 0);


    avformat_free_context(audio_ctx);
    avformat_free_context(video_ctx);

    avformat_close_input(&formatContext);
    avformat_free_context(formatContext);
    avformat_network_deinit();
}
