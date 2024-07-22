#include <iostream>
#include <string>
#include <chrono>

#include "decode.h"
#include "encode.h"
#include "player.h"
#include "playaudio.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#include "VideoState.h"
#include "demuxer.h"
#include "AudioDecoder.h"
#include "VideoDecoder.h"
#include "AudioRender.h"
//#include "VideoR"

int main(int argc, char* argv[]) {
    avformat_network_init();

    auto now = std::chrono::high_resolution_clock::now();


    std::string filename = "./dk.wma";
    // audio.aac
    //std::string filename = "./audio.aac";

    //std::string filename = "jay.mp3";
    //std::string filename = "./test.mp4";
    //FFmpegStudy::splite_audio_video(filename);
    //if (AVFormatContext* ctx = FFmpegStudy::open_file(filename.c_str()))
    //{
    //    auto index = FFmpegStudy::get_audio_vedio_index(ctx);
    //    FFmpegStudy::decode(ctx, std::get<1>(index));
    //    FFmpegStudy::close(ctx);
    //}
    //FFmpegStudyEncode::encode("./333.mp4");
    //FFmpegStudyPlayer::player(filename.c_str());
    //FFmpegStudyPlayer::player(filename.c_str());
    //FFmpegStudyPlayer_Audio::player(filename.c_str());



    VideoState *vs = new VideoState();

    Demuxer* demuxer = new Demuxer(filename.c_str());
    demuxer->set_video_state(vs);

    AudioDecoder* audio_decoder = new AudioDecoder();
    audio_decoder->set_video_state(vs);

    AudioRender* audio_render = new AudioRender();
    audio_render->set_video_state(vs);

    demuxer->start();
    vs->start();
    audio_decoder->start();
    audio_render->start();

    while (1)
    {
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - now) << std::endl;

    return 0;
}
