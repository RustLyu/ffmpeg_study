#include <iostream>
#include <condition_variable>

#include "AudioRender.h"
#include "VideoState.h"
#include "SDL.h"
#include "common.h"

static void audio_callback_render(void* userdata, Uint8* stream, int len) {
    if (!userdata || !stream || len <= 0) {
        return;
    }

    auto vs = static_cast<VideoState*>(userdata);
    auto buf = vs->get_audio_buffer();
    if (!buf) {
        return;
    }

    auto buffer_len = buf->size();
    if (buffer_len <= 0) {
        SDL_memset(stream, 0, len);
        return;
    }
    SDL_memset(stream, 0, len);
    len = std::min(len, buffer_len);
    static std::vector<char> buffer;
    buffer.resize(len);
    
    buf->read(buffer.data(), len);
    SDL_MixAudioFormat(stream, reinterpret_cast<Uint8*>(buffer.data()), AUDIO_S16SYS, len, SDL_MIX_MAXVOLUME);

#ifdef _DEBUG
    //std::cout << "Audio buffer read: " << len << " bytes" << std::endl;
#endif
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

    SDL_AudioDeviceID device_id = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(0, 0), false, &wanted_spec, 
        &spec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    SDL_PauseAudioDevice(device_id, 0);
    return 0;
}

void AudioRender::set_video_state(VideoState* vs)
{
	vs_ = vs;
}
