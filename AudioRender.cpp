#include <iostream>
#include <condition_variable>

#include "AudioRender.h"
#include "VideoState.h"
#include "SDL.h"
#include "common.h"

static void audio_callback_render(void* userdata, Uint8* stream, int len) {
    auto vs = (VideoState*)(userdata);
    auto buf = vs->get_audio_buffer();
    SDL_memset(stream, 0, len);
    std::unique_lock<std::mutex> m(buf->m);
    auto buffer_len = buf->size - buf->read_index;
    if (buf->size <= 0)
    {
        return;
    }
    len = (len > buffer_len ? buffer_len : len);
    //std::cout << " read:" << len << std::endl;
    SDL_memset(stream, 0, len);
    SDL_MixAudioFormat(stream, (uint8_t*)buf->buffer + buf->read_index, AUDIO_S16SYS, len, SDL_MIX_MAXVOLUME);
    buf->read_index += len;
    if (buf->size == buf->read_index)
        buf->cv.notify_all();
}

AudioRender::AudioRender():vs_(nullptr)
{
}

AudioRender::~AudioRender()
{
}

int AudioRender::start()
{
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        LOG("Could not initialize SDL - %s", SDL_GetError());
        return -1;
    }

    int deviceCount = SDL_GetNumAudioDevices(0);
    LOG("SDL_GetNumAudioDevices %d\n", deviceCount);
    for (int i = 0; i < deviceCount; i++) {
        LOG("Audio Device %d: %s\n", i, SDL_GetAudioDeviceName(i, 0));
    }

    SDL_AudioSpec wanted_spec, spec;
    wanted_spec.freq = vs_->get_audio_param().codec_ctx->sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = vs_->get_audio_param().codec_ctx->ch_layout.nb_channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = 1024;
    wanted_spec.callback = audio_callback_render;
    wanted_spec.userdata = vs_;

    SDL_AudioDeviceID device_id = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(0, 0), false, &wanted_spec, &spec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    SDL_PauseAudioDevice(device_id, 0);
}

void AudioRender::set_video_state(VideoState* vs)
{
	vs_ = vs;
}
