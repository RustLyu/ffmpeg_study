//#include <iostream>
//#include <vector>
//#include "common.h"
//#include "playaudio.h"
//
//extern "C" {
//#include <libavformat/avformat.h>
//#include <libavcodec/avcodec.h>
//#include <libavutil/avutil.h>
//#include <libswresample/swresample.h>
//}
//#include <SDL.h>
//
//#include <iostream>
//
//#define SDL_AUDIO_BUFFER_SIZE 1024
//
//struct AudioData {
//    uint8_t* pos;
//    int length;
//};
//
//void audio_callback_audio(void* userdata, Uint8* stream, int len) {
//    AudioData* audio = (AudioData*)userdata;
//    if (audio->length <= 0)
//    {
//        std::cout << "len error" << std::endl;
//        return;
//    }
//
//    len = (len > audio->length ? audio->length : len);
//    //SDL_memcpy(stream, audio->pos, len);
//    SDL_MixAudio(stream, audio->pos, len, SDL_MIX_MAXVOLUME);
//    audio->pos += len;
//    audio->length -= len;
//    //SDL_memset(stream, 0, len); // 将缓冲区清零
//    //if (audio->length <= 0) {
//    //    return;
//    //}
//    //while (len > 0) { // 每次都要凑足len个字节才能退出循环
//    //    int fill_len = (len > audio->length ? audio->length : len);
//    //    // 将音频数据混合到缓冲区
//    //    SDL_MixAudio(stream, audio->pos, fill_len, SDL_MIX_MAXVOLUME);
//    //    audio->pos += fill_len;
//    //    audio->length -= fill_len;
//    //    len -= fill_len;
//    //    stream += fill_len;
//    //    if (audio->length == 0) { // 这里要延迟一会儿，避免一直占据IO资源
//    //        SDL_Delay(1);
//    //    }
//    //}
//}
//
//int FFmpegStudyPlayer_Audio::player(const char* path) {
//    //av_register_all();
//
//    //const char* input_filename = "input.mp4";
//    AVFormatContext* fmt_ctx = avformat_alloc_context();
//
//    if (avformat_open_input(&fmt_ctx, path, nullptr, nullptr) < 0) {
//        std::cerr << "Failed to open input file" << std::endl;
//        return -1;
//    }
//
//    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
//        std::cerr << "Failed to find stream info" << std::endl;
//        return -1;
//    }
//
//    int audio_stream_index = -1;
//    for (unsigned i = 0; i < fmt_ctx->nb_streams; ++i) {
//        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
//            audio_stream_index = i;
//            break;
//        }
//    }
//
//    if (audio_stream_index == -1) {
//        std::cerr << "Failed to find audio stream" << std::endl;
//        return -1;
//    }
//
//    AVCodecParameters* codecpar = fmt_ctx->streams[audio_stream_index]->codecpar;
//    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
//    if (!codec) {
//        std::cerr << "Failed to find decoder" << std::endl;
//        return -1;
//    }
//
//    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
//    if (!codec_ctx) {
//        std::cerr << "Failed to allocate codec context" << std::endl;
//        return -1;
//    }
//
//    if (avcodec_parameters_to_context(codec_ctx, codecpar) < 0) {
//        std::cerr << "Failed to copy codec parameters to context" << std::endl;
//        return -1;
//    }
//
//    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
//        std::cerr << "Failed to open codec" << std::endl;
//        return -1;
//    }
//
//    SwrContext* swr_ctx = swr_alloc();
//        swr_alloc_set_opts2(&swr_ctx, &codec_ctx->ch_layout, AV_SAMPLE_FMT_S16,
//            codec_ctx->sample_rate, &codec_ctx->ch_layout, codec_ctx->sample_fmt,
//            codec_ctx->sample_rate, 0, nullptr);
//        swr_init(swr_ctx);
//
//    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
//        std::cerr << "Failed to initialize SDL" << std::endl;
//        return -1;
//    }
//    int deviceCount = SDL_GetNumAudioDevices(0);
//    printf("SDL_GetNumAudioDevices %d\n", deviceCount);
//    for (int i = 0; i < deviceCount; i++) {
//        printf("Audio Device %d: %s\n", i, SDL_GetAudioDeviceName(i, 0));
//    }
//
//    SDL_AudioSpec wanted_spec, spec;
//    wanted_spec.freq = codec_ctx->sample_rate;
//    wanted_spec.format = AUDIO_S16SYS;
//    wanted_spec.channels = 2;
//    wanted_spec.silence = 0;
//    wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
//    wanted_spec.callback = audio_callback_audio;
//    AudioData audio_data = { nullptr, 0 };
//    wanted_spec.userdata = &audio_data;
//
//    //SDL_AudioDeviceID device_id = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(1, 0), false, &spec, nullptr, false);
//    SDL_AudioDeviceID device_id = SDL_OpenAudioDevice(nullptr, false, &spec, nullptr, false);
//    /*if (SDL_OpenAudio(&wanted_spec, &spec) < 0) {
//        std::cerr << "Failed to open audio device" << std::endl;
//        return -1;
//    }*/
//
//    AVPacket packet;
//    AVFrame* frame = av_frame_alloc();
//    uint8_t* audio_buf = nullptr;
//    //int audio_buf_size = 0;
//
//    //SDL_PauseAudio(0);
//    SDL_PauseAudioDevice(device_id, 0);
//    std::vector<uint8_t> out_buffer;
//    while (av_read_frame(fmt_ctx, &packet) >= 0) {
//        if (packet.stream_index == audio_stream_index) {
//            if (avcodec_send_packet(codec_ctx, &packet) >= 0) {
//                while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
//                    /*int dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, codec_ctx->sample_rate) + frame->nb_samples,
//                        codec_ctx->sample_rate, codec_ctx->sample_rate, AV_ROUND_UP);
//
//                    int len2 = swr_convert(swr_ctx, &audio_buf, dst_nb_samples, (const uint8_t**)frame->data, frame->nb_samples);
//                    if (len2 < 0) {
//                        std::cerr << "Error while converting" << std::endl;
//                        break;
//                    }
//
//                    audio_buf_size = av_samples_get_buffer_size(nullptr, 2, len2, AV_SAMPLE_FMT_S16, 1);
//                    if (audio_buf_size < 0) {
//                        std::cerr << "Error while calculating buffer size" << std::endl;
//                        break;
//                    }*/
//
//                    
//
//
//                    //av_freep(&audio_buf);
//                    //int out_samples = av_rescale_rnd(swr_get_delay(swr_ctx, 48000) +
//                    //    codec_ctx->sample_rate, 44100, 48000, AV_ROUND_UP);
//                    //std::cout << codec_ctx->frame_size << std::endl;
//                    //av_samples_alloc(&audio_buf, NULL, 2, out_samples,
//                    //    AV_SAMPLE_FMT_S16, 0);
//                    //out_samples = swr_convert(swr_ctx, &audio_buf, out_samples,
//                    //    (const uint8_t**)frame->data, frame->nb_samples);
//                    ////handle_output(output, out_samples);
//                    ////av_freep(&output);
//                    //int audio_buf_size = av_samples_get_buffer_size(nullptr, 2, out_samples, AV_SAMPLE_FMT_S16, 1);
//                    ////std::cout << "OOOOOOOOOOOOOO:" << out_samples << std::endl;
//                    ////if (audio_buf_size < 0)
//                    ////    std::cout << "eeeeeeeeeeeee:" << out_samples << std::endl;
//                    ////else {
//                    ////    std::cout << "OOOOOOOOOOOOOO:" << out_samples << std::endl;
//                    ////}
//                    //std::cout << audio_buf_size << std::endl;
//                    //audio_data.pos = audio_buf;
//                    //audio_data.length = audio_buf_size;
//
//                    //while (audio_data.length > 0) {
//                    //    SDL_Delay(1);
//                    //}
//                    int out_nb_samples = swr_get_out_samples(swr_ctx, frame->nb_samples);
//                    out_buffer.resize(av_samples_get_buffer_size(NULL, codec_ctx->ch_layout.nb_channels, out_nb_samples, codec_ctx->sample_fmt, 1));
//                    uint8_t* out = out_buffer.data();
//
//                    int ret = swr_convert(swr_ctx, &out, out_nb_samples, (const uint8_t**)frame->data,
//                        frame->nb_samples);
//                    if (ret < 0) {
//                        printf("swr_convert failed %d\n", ret);
//                    }
//                    int out_samples = ret;
//                    SDL_QueueAudio(device_id, out, out_samples * spec.channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16));
//                }
//            }
//        }
//        av_packet_unref(&packet);
//    }
//    SDL_Delay(2000000); // 等音频播完
//    av_free(audio_buf);
//    av_frame_free(&frame);
//    avcodec_free_context(&codec_ctx);
//    avformat_close_input(&fmt_ctx);
//    swr_free(&swr_ctx);
//
//    SDL_CloseAudioDevice(device_id);
//    SDL_Quit();
//
//    return 0;
//}




#include <cstdio>
#include <string>
#include <fstream>
#include <vector>

#define SDL_MAIN_HANDLED
#include "SDL.h"

extern "C" {
#include "libswresample/swresample.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

AVFormatContext* OpenAudioFromFile(const std::string& file_path) {
    AVFormatContext* format_ctx = avformat_alloc_context();
    if (int ret = avformat_open_input(&format_ctx, file_path.c_str(), NULL, NULL);
        ret < 0) {
        printf("avformat_open_input failed, ret: %d, path: %s\n", ret, file_path.c_str());
    }
    return format_ctx;
}

#include "playaudio.h"
#include <iostream>

#define SDL_AUDIO_BUFFER_SIZE 1024

struct AudioData {
    uint8_t* pos;
    int length;
};

#include <mutex>
std::mutex m;
void audio_callback_audio(void* userdata, Uint8* stream, int len) {
    AudioData* audio = (AudioData*)userdata;
    if (audio->length <= 0)
    {
        //std::cout << "len error" << std::endl;
        return;
    }
    std::cout << "read:" << len << std::endl;
    len = (len > audio->length ? audio->length : len);
    SDL_memset(stream, 0, len);
    SDL_MixAudio(stream, audio->pos, len, SDL_MIX_MAXVOLUME);
    //SDL_MixAudioFormat(stream, (uint8_t*)audio->pos, AUDIO_S16SYS, len, SDL_MIX_MAXVOLUME);
    //std::lock_guard<std::mutex> lock(m);
    audio->pos += len;
    audio->length -= len;
    /*AudioData* audio_data = (AudioData*)userdata;

    if (audio_data->length <= 0)
        return;

    len = (len > audio_data->length ? audio_data->length : len);
    SDL_memcpy(stream, audio_data->pos, len);

    audio_data->pos += len;
    audio_data->length -= len;*/
}

int FFmpegStudyPlayer_Audio::player(const char* path) {
    printf("ffmpeg version: %s\n", av_version_info());

    SDL_version version;
    SDL_GetVersion(&version);
    printf("SDL version: %d.%d.%d\n", version.major, version.minor, version.patch);

    //char* file = "D:/qytx.mp3";
    AVFormatContext* pFormatCtx = NULL; //for opening multi-media file

    int audioStream = -1;

    AVCodecParameters* pCodecParameters = NULL; //codec context
    AVCodecContext* pCodecCtx = NULL;

    const AVCodec* pCodec = NULL; // the codecer
    AVFrame* pFrame = NULL;
    AVPacket* packet;

    int64_t in_channel_layout;
    struct SwrContext* au_convert_ctx;

    pFormatCtx = OpenAudioFromFile(path);
    //    std::string data = readFile(file);
    if (!pFormatCtx) {
        return -1;
    }

    if (avformat_find_stream_info(pFormatCtx, nullptr) < 0) {
        return -1;
    }

    audioStream = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    printf("av_find_best_stream %d\n", audioStream);

    if (audioStream == -1) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Din't find a video stream!");
        return -1;// Didn't find a video stream
    }

    // Get a pointer to the codec context for the video stream
    pCodecParameters = pFormatCtx->streams[audioStream]->codecpar;

    // Find the decoder for the video stream
    pCodec = avcodec_find_decoder(pCodecParameters->codec_id);
    if (pCodec == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unsupported codec!\n");
        return -1; // Codec not found
    }

    // Copy context
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (avcodec_parameters_to_context(pCodecCtx, pCodecParameters) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't copy codec context");
        return -1;// Error copying codec context
    }

    // Open codec
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open decoder!\n");
        return -1; // Could not open codec
    }
    packet = (AVPacket*)av_malloc(sizeof(AVPacket));
    av_init_packet(packet);
    pFrame = av_frame_alloc();

    uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO; 
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16; 
    int out_sample_rate = 44100;
    int out_channels = 2;

    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }

    int deviceCount = SDL_GetNumAudioDevices(0);
    printf("SDL_GetNumAudioDevices %d\n", deviceCount);
    for (int i = 0; i < deviceCount; i++) {
        printf("Audio Device %d: %s\n", i, SDL_GetAudioDeviceName(i, 0));
    }

    SDL_AudioSpec wanted_spec, spec;
    wanted_spec.freq = pCodecCtx->sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = pCodecCtx->ch_layout.nb_channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = 1024;
    wanted_spec.callback = nullptr;
    wanted_spec.callback = audio_callback_audio;
    AudioData audio_data = { nullptr, 0 };
    wanted_spec.userdata = &audio_data;

    //SDL_AudioDeviceID device_id = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(0, 0), false, &spec, nullptr, false);
    int d = SDL_OpenAudio(&wanted_spec, &spec);
    //printf("device_id %d\n", device_id);
    /*if (device_id == 0) {
        printf("can't open audio.\n");
        return -1;
    }*/

    au_convert_ctx = swr_alloc();
    swr_alloc_set_opts2(&au_convert_ctx, &pCodecCtx->ch_layout, AV_SAMPLE_FMT_S16,
        pCodecCtx->sample_rate, &pCodecCtx->ch_layout, pCodecCtx->sample_fmt,
        pCodecCtx->sample_rate, 0, nullptr);

    swr_init(au_convert_ctx);

    //SDL_PauseAudioDevice(device_id, 0);
    SDL_PauseAudio(0);
    fflush(stdout);

    uint8_t* out_buffer = (uint8_t*)av_malloc(192000);
    int out_buffer_size;

    SDL_PauseAudio(0);
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        if (packet->stream_index == audioStream) {
            avcodec_send_packet(pCodecCtx, packet);
            while (auto ret = avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                }
                else if (ret < 0) {
                    break;
                }
                //std::lock_guard<std::mutex> lock(m);
                int dst_nb_samples = swr_get_out_samples(au_convert_ctx, pFrame->nb_samples);
                ////out_buffer.resize(av_samples_get_buffer_size(NULL, pCodecCtx->ch_layout.nb_channels, dst_nb_samples, pCodecCtx->sample_fmt, 1));
                /*uint8_t* out = out_buffer;
                int len2 = swr_convert(au_convert_ctx, &out, dst_nb_samples, (const uint8_t**)pFrame->data, pFrame->nb_samples);
                if (len2 < 0) {
                    std::cerr << "Error while converting" << std::endl;
                    break;
                }
                audio_data.pos = out;
                audio_data.length = len2 * spec.channels * av_get_bytes_per_sample(out_sample_fmt);

                while (audio_data.length > 0) {
                    SDL_Delay(1);
                }*/
                out_buffer_size = av_samples_get_buffer_size(NULL, pCodecCtx->ch_layout.nb_channels,
                    dst_nb_samples, AV_SAMPLE_FMT_S16, 1);
                swr_convert(au_convert_ctx, &out_buffer, out_buffer_size,
                    (const uint8_t**)pFrame->data, pFrame->nb_samples);
                audio_data.pos = out_buffer;
                audio_data.length = out_buffer_size;
                std::cout << "write:" << out_buffer_size << std::endl;
                while (audio_data.length > 0) {
                    SDL_Delay(1);
                }
            }
        }
        av_packet_unref(packet);
    }
    //SDL_Delay(300000);
    swr_free(&au_convert_ctx);
    SDL_CloseAudio();
    SDL_Quit();
}
