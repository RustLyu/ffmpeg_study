#include <iostream>

#include "common.h"
#include "player.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
}
#include <SDL.h>

//const int SDL_AUDIO_BUFFER_SIZE = 1024;
//
//// 音频缓冲区
//#define buffer_size 1920000
//
//uint8_t* audio_buf = nullptr;
////uint8_t* audio_buf = nullptr;
//unsigned int audio_buf_size = 0;
//unsigned int audio_buf_index = 0;
//
//void audio_callback(void* userdata, uint8_t* stream, int len) {
//	/*std::cout << "audio_buf_index: " << audio_buf_index << std::endl;
//	if (audio_buf_index >= audio_buf_size) {
//		return;
//	}
//
//	len = (len > audio_buf_size - audio_buf_index) ? audio_buf_size - audio_buf_index : len;
//	SDL_memcpy(stream, (uint8_t*)audio_buf + audio_buf_index, len);
//	audio_buf_index += len;*/
//
//	SDL_memset(stream, 0, len);
//	//printf("render audio, last: %ld\n", render->audio_queue_.size());
//	SDL_MixAudio(stream, audio_buf, audio_buf_size, SDL_MIX_MAXVOLUME);
//
//	//if (auto master = render->master_.lock()) {
//	//	printf("[sfplayer]commmit audio pts:%lld\n", frame->pts);
//	//	master->UpdateLastAudioPts(frame->pts);
//	//}
//}

#define SDL_AUDIO_BUFFER_SIZE 1024

struct AudioData {
	uint8_t* pos;
	int length;
};

void audio_callback(void* userdata, Uint8* stream, int len) {
	AudioData* audio = (AudioData*)userdata;
	if (audio->length == 0)
		return;

	len = (len > audio->length ? audio->length : len);
	SDL_memcpy(stream, audio->pos, len);
	audio->pos += len;
	audio->length -= len;
}

int FFmpegStudyPlayer::player(const char* path)
{
	AVFormatContext* ctx = avformat_alloc_context();

	if (avformat_open_input(&ctx, path, nullptr, nullptr) < 0)
		return -1;

	if (avformat_find_stream_info(ctx, nullptr) < 0)
		return -1;

	int video_index = -1;
	int audio_index = -1;

	const AVCodec* cc = nullptr;
	auto kk = av_find_best_stream(ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &cc, 0);

	for (int i = 0; i < ctx->nb_streams; ++i)
	{
		if (ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			video_index = i;
		}
		if (ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audio_index = i;
		}
	}

	if (video_index < 0 && audio_index < 0)
		return -1;

	AVCodecContext* codec_ctx = nullptr;
	if (video_index >= 0) {
		AVCodecParameters* param = ctx->streams[video_index]->codecpar;
		const AVCodec* codec = avcodec_find_decoder(ctx->streams[video_index]->codecpar->codec_id);
		codec_ctx = avcodec_alloc_context3(codec);
		if (avcodec_parameters_to_context(codec_ctx, param) < 0)
			return -1;
		if (avcodec_open2(codec_ctx, codec, nullptr) < 0)
			return -1;
	}

	AVFrame* audio_frame = nullptr;
	AVCodecContext* audio_codec_ctx = nullptr;
	if (audio_index != -1) {
		audio_frame = av_frame_alloc();
		AVCodecParameters* param = ctx->streams[audio_index]->codecpar;
		const AVCodec* codec = avcodec_find_decoder(ctx->streams[audio_index]->codecpar->codec_id);
		audio_codec_ctx = avcodec_alloc_context3(codec);
		if (avcodec_parameters_to_context(audio_codec_ctx, param) < 0)
			return -1;
		if (avcodec_open2(audio_codec_ctx, codec, nullptr) < 0)
			return -1;
	}


	AVFrame* frame = nullptr; 
	AVFrame* frameYUV = nullptr;
	if (video_index != -1)
	{
		frame = av_frame_alloc();
		frameYUV = av_frame_alloc();
		uint8_t* buffer = nullptr;
		int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, codec_ctx->width,
			codec_ctx->height, 32);
		buffer = (uint8_t*)av_malloc(num_bytes * sizeof(uint8_t));
		av_image_fill_arrays(frameYUV->data, frameYUV->linesize, buffer, AV_PIX_FMT_YUV420P,
			codec_ctx->width, codec_ctx->height, 32);
	}
	
	struct SwsContext* sws = nullptr;
	if(video_index != -1)
		sws = sws_getContext(codec_ctx->width, codec_ctx->height,
		codec_ctx->pix_fmt, codec_ctx->width, codec_ctx->height, AV_PIX_FMT_YUV420P, SWS_BILINEAR, nullptr, nullptr, nullptr);


	struct SwrContext* swr_ctx = nullptr;
	if (audio_index != -1) {
		swr_alloc_set_opts2(&swr_ctx, &audio_codec_ctx->ch_layout, AV_SAMPLE_FMT_S16,
			audio_codec_ctx->sample_rate, &audio_codec_ctx->ch_layout, audio_codec_ctx->sample_fmt,
			audio_codec_ctx->sample_rate, 0, nullptr);
		swr_init(swr_ctx);
	}

	if (SDL_Init(SDL_INIT_VIDEO) && SDL_Init(SDL_INIT_AUDIO)) {
		return -1;
	}


	SDL_AudioSpec wanted_spec, spec;


	AudioData audio_data = { nullptr, 0 };

	if (audio_index >= 0)
	{
		wanted_spec.freq = audio_codec_ctx->sample_rate;
		wanted_spec.format = AUDIO_S16SYS;
		wanted_spec.channels = 2;// audio_codec_ctx->ch_layout.nb_channels;
		wanted_spec.silence = 0;
		wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
		wanted_spec.callback = audio_callback;
		wanted_spec.userdata = &audio_data;

		if (SDL_OpenAudio(&wanted_spec, &spec) < 0) {
			return -1;
		}
		SDL_PauseAudio(0);
	}

	SDL_Window* win = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_Texture* texture = nullptr;
	if (video_index >= 0)
	{
		win = SDL_CreateWindow("test", SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED, codec_ctx->width, codec_ctx->height, SDL_WINDOW_OPENGL);
		renderer = SDL_CreateRenderer(win, -1, 0);
		 texture = SDL_CreateTexture(renderer,
			SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, codec_ctx->width, codec_ctx->height);
	} 

	SDL_Rect rect;
	if (video_index != -1)
	{
		rect.x = 0;
		rect.y = 0;
		rect.w = codec_ctx->width;
		rect.h = codec_ctx->height;
	}
	
	AVPacket* pkt = av_packet_alloc();
	int ret = 0;
	SDL_Event event;

	//int tt = av_samples_get_buffer_size(nullptr, audio_codec_ctx->ch_layout.nb_channels, 192000, AV_SAMPLE_FMT_S16, 1);
	//
	////audio_buf_size = buffer_size;
	//audio_buf_index = 0;
	uint8_t* audio_buf = (uint8_t*)av_malloc(192000);
	int audio_buf_size = 0;

	SDL_PauseAudio(0);

	while (av_read_frame(ctx, pkt) >= 0)
	{
		if (pkt->stream_index == video_index) {

			ret = avcodec_send_packet(codec_ctx, pkt);

			if (ret < 0)
				return -1;
			while (ret >= 0)
			{
				ret = avcodec_receive_frame(codec_ctx, frame);
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
					break;
				else if (ret < 0)
					return -1;
				sws_scale(sws, (uint8_t const* const*)frame->data, frame->linesize, 0, 
					codec_ctx->height, frameYUV->data, frameYUV->linesize);

				SDL_UpdateYUVTexture(texture, &rect, frameYUV->data[0], frameYUV->linesize[0],
					frameYUV->data[1], frameYUV->linesize[1], frameYUV->data[2], frameYUV->linesize[2]);

				SDL_RenderClear(renderer);
				SDL_RenderCopy(renderer, texture, nullptr, &rect);
				SDL_RenderPresent(renderer);

				SDL_PollEvent(&event);
				switch (event.type) {
				case SDL_QUIT:
					SDL_Quit();
					return 0;
				default:
					break;
				}

				SDL_Delay(40); // To simulate 25 fps
			}
		}
		else if (pkt->stream_index == audio_index) {
			ret = avcodec_send_packet(audio_codec_ctx, pkt);
			if (ret < 0) {
				break;
			}

			while (ret >= 0) {
				ret = avcodec_receive_frame(audio_codec_ctx, audio_frame);
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
					break;
				}
				else if (ret < 0) {
					break;
				}

				/*int dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, audio_codec_ctx->sample_rate) + audio_frame->nb_samples,
					audio_codec_ctx->sample_rate, audio_codec_ctx->sample_rate, AV_ROUND_UP);
				std::cout << "dst_nb_samples: " << dst_nb_samples << std::endl;
				int len2 = swr_convert(swr_ctx, (uint8_t**)&audio_buf, dst_nb_samples, (const uint8_t**)audio_frame->data, audio_frame->nb_samples);
				audio_buf_size = av_samples_get_buffer_size(nullptr, audio_codec_ctx->ch_layout.nb_channels, len2, AV_SAMPLE_FMT_S16, 1);
				audio_buf_index = 0;*/
				//int dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, audio_codec_ctx->sample_rate) + audio_frame->nb_samples,
				//	audio_codec_ctx->sample_rate, audio_codec_ctx->sample_rate, AV_ROUND_UP);
				//int dst_nb_samples = swr_get_out_samples(swr_ctx, audio_frame->nb_samples);
				//std::cout << " dst_nb_samples : " << dst_nb_samples << std::endl;
				//int audio_buf_size_2 = dst_nb_samples * audio_frame->ch_layout.nb_channels * 2;
				//if (audio_buf != nullptr)
				//{
				//	av_free(audio_buf);
				//	audio_buf = nullptr;
				//}
				//audio_buf = (uint8_t*)av_malloc(audio_buf_size_2);
				//if (!audio_buf) {
				//	std::cerr << "Failed to allocate audio buffer" << std::endl;
				//	exit(1);
				//}
				//memset(audio_buf, 0, audio_buf_size_2);
				//std::cout << "audio_buf_size：" << audio_buf_size_2 << std::endl;
				//int len2 = swr_convert(swr_ctx, &audio_buf, dst_nb_samples, (const uint8_t**)audio_frame->data, audio_frame->nb_samples);
				//std::cout << " len 2 : " << len2 << std::endl;
				////audio_buf_size = av_samples_get_buffer_size(nullptr, audio_codec_ctx->ch_layout.nb_channels, len2, AV_SAMPLE_FMT_S16, 1);
				//audio_buf_index = 0;
				//std::cout << "reset audio_buf_index: "<< std::endl;
				//audio_buf_size = audio_buf_size_2;
				//SDL_PauseAudio(0);
				/*while (audio_buf_index < audio_buf_size) {
					SDL_Delay(1);
				}*/
				int dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, audio_codec_ctx->sample_rate) + audio_frame->nb_samples,
					audio_codec_ctx->sample_rate, audio_codec_ctx->sample_rate, AV_ROUND_UP);

				int len2 = swr_convert(swr_ctx, &audio_buf, dst_nb_samples, (const uint8_t**)audio_frame->data, audio_frame->nb_samples);
				if (len2 < 0) {
					std::cerr << "Error while converting" << std::endl;
					break;
				}

				audio_buf_size = av_samples_get_buffer_size(nullptr, 2, len2, AV_SAMPLE_FMT_S16, 1);
				ERRROR_CODE_2_STR(audio_buf_size);
				audio_data.pos = audio_buf;
				audio_data.length = audio_buf_size;

				while (audio_data.length > 0) {
					SDL_Delay(1);
				}
			}
		}

		av_packet_unref(pkt);
	}

	sws_freeContext(sws);
	av_frame_free(&frame);
	av_frame_free(&frameYUV);
	avcodec_close(codec_ctx);
	avformat_close_input(&ctx);

	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(win);
	SDL_Quit();

	return 0;
}
