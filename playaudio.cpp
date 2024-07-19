#include <cstdio>
#include <string>
#include <fstream>
#include <vector>
#include <iostream>

#include "playaudio.h"

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

#define SDL_AUDIO_BUFFER_SIZE 1024

struct AudioData {
    uint8_t* pos;
    int length;
};

void audio_callback_audio(void* userdata, Uint8* stream, int len) {
    AudioData* audio = (AudioData*)userdata;
    if (audio->length <= 0)
    {
        return;
    }
    len = (len > audio->length ? audio->length : len);
    SDL_memset(stream, 0, len);
    SDL_MixAudioFormat(stream, (uint8_t*)audio->pos, AUDIO_S16SYS, len, SDL_MIX_MAXVOLUME);
    audio->pos += len;
    audio->length -= len;
}

int FFmpegStudyPlayer_Audio::player(const char* path) {
    printf("ffmpeg version: %s\n", av_version_info());

    SDL_version version;
    SDL_GetVersion(&version);
    printf("SDL version: %d.%d.%d\n", version.major, version.minor, version.patch);

    AVFormatContext* fmt_ctx = NULL;

    int audioStream = -1;

    AVCodecParameters* code_param = NULL; 
    AVCodecContext* code_ctx = NULL;

    const AVCodec* audio_codec = NULL;
    AVFrame* audio_frame = NULL;
    AVPacket* packet;

    int64_t in_channel_layout;
    struct SwrContext* au_convert_ctx;

    fmt_ctx = OpenAudioFromFile(path);
    //    std::string data = readFile(file);
    if (!fmt_ctx) {
        return -1;
    }

    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        return -1;
    }

    audioStream = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    printf("av_find_best_stream %d\n", audioStream);

    if (audioStream == -1) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Din't find a video stream!");
        return -1;
    }

    code_param = fmt_ctx->streams[audioStream]->codecpar;

    audio_codec = avcodec_find_decoder(code_param->codec_id);
    if (audio_codec == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unsupported codec!\n");
        return -1;
    }

    code_ctx = avcodec_alloc_context3(audio_codec);
    if (avcodec_parameters_to_context(code_ctx, code_param) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't copy codec context");
        return -1;
    }

    if (avcodec_open2(code_ctx, audio_codec, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open decoder!\n");
        return -1;
    }
    packet = (AVPacket*)av_malloc(sizeof(AVPacket));
    av_init_packet(packet);
    audio_frame = av_frame_alloc();

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
    wanted_spec.freq = code_ctx->sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = code_ctx->ch_layout.nb_channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = 1024;
    wanted_spec.callback = nullptr;
    wanted_spec.callback = audio_callback_audio;
    AudioData audio_data = { nullptr, 0 };
    wanted_spec.userdata = &audio_data;

    SDL_AudioDeviceID device_id = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(0, 0), false, &wanted_spec, &spec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE);

    au_convert_ctx = swr_alloc();
    swr_alloc_set_opts2(&au_convert_ctx, &code_ctx->ch_layout, AV_SAMPLE_FMT_S16,
        code_ctx->sample_rate, &code_ctx->ch_layout, code_ctx->sample_fmt,
        code_ctx->sample_rate, 0, nullptr);

    swr_init(au_convert_ctx);

    SDL_PauseAudioDevice(device_id, 0);

    uint8_t* out_buffer = (uint8_t*)av_malloc(192000);
    int out_buffer_size;
    
    fflush(stdout);

    while (av_read_frame(fmt_ctx, packet) >= 0) {
        if (packet->stream_index == audioStream) {
            avcodec_send_packet(code_ctx, packet);
            while (auto ret = avcodec_receive_frame(code_ctx, audio_frame) == 0) {
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                }
                else if (ret < 0) {
                    break;
                }
                int dst_nb_samples = swr_get_out_samples(au_convert_ctx, audio_frame->nb_samples);
                out_buffer_size = av_samples_get_buffer_size(NULL, code_ctx->ch_layout.nb_channels,
                    dst_nb_samples, AV_SAMPLE_FMT_S16, 1);
                swr_convert(au_convert_ctx, &out_buffer, out_buffer_size,
                    (const uint8_t**)audio_frame->data, audio_frame->nb_samples);
                audio_data.pos = out_buffer;
                audio_data.length = out_buffer_size;
                while (audio_data.length > 0) {
                    SDL_Delay(1);
                }
            }
        }
        av_packet_unref(packet);
    }
    SDL_Delay(300000);
    swr_free(&au_convert_ctx);
    SDL_CloseAudioDevice(device_id);
    SDL_Quit();
}
