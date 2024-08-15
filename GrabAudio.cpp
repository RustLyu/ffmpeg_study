#include "GrabAudio.h"
#include "common.h"
#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavdevice/avdevice.h>
#include "libavutil/avassert.h"
#include "libswresample/swresample.h"
}

bool FFmpeg_Grab_Audio::Grab::stop_ = false;
int FFmpeg_Grab_Audio::Grab::grab(const char* path)
{
	av_log_set_level(AV_LOG_TRACE);
	avdevice_register_all();
	avformat_network_init();

	/*************************IN PUT AUDIO *******************************/
	AVFormatContext* in_audio_ctx = avformat_alloc_context();
	if (in_audio_ctx == nullptr)
		return -1;

	AVDictionary* options = NULL;
	char fpsChar[3];
	int m_fps = 30;
	itoa(m_fps, fpsChar, 10);
	av_dict_set(&options, "framerate", fpsChar, 0);

	const AVInputFormat* input_fmt = av_find_input_format("dshow");
	int ret = avformat_open_input(&in_audio_ctx, "audio=virtual-audio-capturer", input_fmt, nullptr);
	if (ret < 0)
	{
		std::cout << "avformat_open_input:" << ret << std::endl;
		return -2;
	}
	if (avformat_find_stream_info(in_audio_ctx, nullptr) < 0)
	{
		std::cout << "avformat_find_stream_info:" << ret << std::endl;
		return -3;
	}

	int audio_index = -1;

	for (int i = 0; i < in_audio_ctx->nb_streams; ++i)
	{
		if (in_audio_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audio_index = i;
			break;
		}
	}

	if (audio_index < 0)
	{
		std::cout << "can not find audio stream:" << std::endl;
		return -4;
	}


	const AVCodec* in_audio_codec = avcodec_find_decoder(in_audio_ctx->streams[audio_index]->codecpar->codec_id);
	AVCodecContext* in_codec_ctx = avcodec_alloc_context3(in_audio_codec);

	if (avcodec_parameters_to_context(in_codec_ctx, in_audio_ctx->streams[audio_index]->codecpar) < 0)
		return -4;
	if (avcodec_open2(in_codec_ctx, in_audio_codec, nullptr) < 0)
		return -5;

	/*************************OUT PUT AUDIO *******************************/
	AVFormatContext* out_audio_ctx = nullptr;
	avformat_alloc_output_context2(&out_audio_ctx, nullptr, "adts", path);
	if (out_audio_ctx == nullptr)
		return -6;
	AVStream* out_audio_stream = avformat_new_stream(out_audio_ctx, nullptr);

	out_audio_stream->id = 0;
	const AVCodec* out_audio_codec = avcodec_find_encoder(out_audio_ctx->oformat->audio_codec);
	AVCodecContext* out_codec_ctx = avcodec_alloc_context3(out_audio_codec);

	AVChannelLayout tempAVChannelLayout = AV_CHANNEL_LAYOUT_STEREO;
	out_codec_ctx->sample_fmt = out_audio_codec->sample_fmts[0];
	out_codec_ctx->bit_rate = 64000;
	out_codec_ctx->sample_rate = 48000;
	if (in_audio_codec->supported_samplerates) {
		out_codec_ctx->sample_rate = in_audio_codec->supported_samplerates[0];
		for (int i = 0; in_audio_codec->supported_samplerates[i]; i++) {
			if (in_audio_codec->supported_samplerates[i] == 48000)
				out_codec_ctx->sample_rate = 48000;
		}
	}
	if (av_channel_layout_copy(&out_codec_ctx->ch_layout, &tempAVChannelLayout) != 0)
	{
		return -9;
	}
	out_audio_stream->time_base = { 1, out_codec_ctx->sample_rate };
	if (out_audio_ctx->oformat->flags & AVFMT_GLOBALHEADER)
		out_audio_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	ret = avcodec_open2(out_codec_ctx, out_audio_codec, nullptr);
	if (ret < 0)
		return -10;

	int nb_samples = 0;
	if (out_audio_codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
		nb_samples = 10000;
	else
		nb_samples = out_codec_ctx->frame_size;

	if (avcodec_parameters_from_context(out_audio_stream->codecpar, out_codec_ctx) < 0) {
		return -12;
	}

	SwrContext* swr_ctx = swr_alloc();
	if (!swr_ctx)
	{
		return -12;
	}

	av_opt_set_chlayout(swr_ctx, "in_chlayout", &out_codec_ctx->ch_layout, 0);
	av_opt_set_int(swr_ctx, "in_sample_rate", out_codec_ctx->sample_rate, 0);
	av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
	av_opt_set_chlayout(swr_ctx, "out_chlayout", &out_codec_ctx->ch_layout, 0);
	av_opt_set_int(swr_ctx, "out_sample_rate", out_codec_ctx->sample_rate, 0);
	av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", out_codec_ctx->sample_fmt, 0);

	if ((ret = swr_init(swr_ctx)) < 0)
	{
		return -13;
	}
	
	av_dump_format(out_audio_ctx, 0, path, 1);

	if (!(out_audio_ctx->oformat->flags & AVFMT_NOFILE)) {
		if (avio_open(&out_audio_ctx->pb, path, AVIO_FLAG_WRITE) < 0) {
			return -7;
		}
	}

	ret = avformat_write_header(out_audio_ctx, nullptr);
	if (ret < 0) {
		ERRROR_CODE_2_STR(ret);
		return -8;
	}


	AVFrame* tmp_frame = av_frame_alloc();
	tmp_frame->format = out_codec_ctx->sample_fmt;
	av_channel_layout_copy(&tmp_frame->ch_layout, &out_codec_ctx->ch_layout);
	tmp_frame->sample_rate = out_codec_ctx->sample_rate;
	tmp_frame->nb_samples = nb_samples;

	AVFrame* in_frame = av_frame_alloc();
	in_frame->format = out_codec_ctx->sample_fmt;
	av_channel_layout_copy(&in_frame->ch_layout, &out_codec_ctx->ch_layout);
	in_frame->sample_rate = out_codec_ctx->sample_rate;
	in_frame->nb_samples = nb_samples;

	AVFrame* out_frame = av_frame_alloc();
	out_frame->format = AV_SAMPLE_FMT_S16;
	av_channel_layout_copy(&out_frame->ch_layout, &out_codec_ctx->ch_layout);
	out_frame->sample_rate = out_codec_ctx->sample_rate;
	out_frame->nb_samples = nb_samples;

	ret = av_frame_get_buffer(tmp_frame, 0);
	ret = av_frame_get_buffer(in_frame, 0);
	ret = av_frame_get_buffer(out_frame, 0);

	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = nullptr;
	pkt.size = 0;
	uint64_t last_pts = 0;
	AVPacket enc_pkt;

	uint8_t* out_buffer = (uint8_t*)av_malloc(192000);
	int size = 0;
	while (!stop_)
	{
		if (av_read_frame(in_audio_ctx, &pkt) < 0)
		{
			break;
		}
		if (pkt.stream_index == audio_index)
		{
			ret = avcodec_send_packet(in_codec_ctx, &pkt);
			if (ret < 0)
			{
				continue;
			}

			while (avcodec_receive_frame(in_codec_ctx, out_frame) == 0)
			{

				memcpy_s(out_buffer + size, out_frame->linesize[0], out_frame->data[0], out_frame->linesize[0]);
				size += out_frame->linesize[0];

				int frameBufSize = av_samples_get_buffer_size(NULL, out_codec_ctx->ch_layout.nb_channels, out_codec_ctx->frame_size, AV_SAMPLE_FMT_S16, 1);
				if ((size) >= frameBufSize)
				{
					unsigned char* frameBuf = new unsigned char[frameBufSize];


					memcpy_s(tmp_frame->data[0], tmp_frame->linesize[0], out_buffer, frameBufSize);
					tmp_frame->pts = last_pts;

					delete [] frameBuf;

					{
						uint8_t* tmp_out_buffer = (uint8_t*)av_malloc(size - frameBufSize);
						memcpy(tmp_out_buffer, out_buffer + frameBufSize, size - frameBufSize);
						memset(out_buffer, 0, 19200);
						memcpy(out_buffer, tmp_out_buffer, size - frameBufSize);
						size -= frameBufSize;
						av_free(tmp_out_buffer);
					}

					//int pcmsize = out_frame->linesize[0];

					int dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, out_codec_ctx->sample_rate) + in_frame->nb_samples,
						out_codec_ctx->sample_rate, out_codec_ctx->sample_rate, AV_ROUND_UP);
					av_frame_make_writable(in_frame);

					ret = swr_convert(swr_ctx,
						in_frame->data, dst_nb_samples,
						(const uint8_t**)tmp_frame->data, tmp_frame->nb_samples);
					if (ret < 0)
						continue;
					in_frame->pts = av_rescale_q(last_pts, { 1, out_codec_ctx->sample_rate }, out_codec_ctx->time_base);
					last_pts += dst_nb_samples;
					ret = avcodec_send_frame(out_codec_ctx, in_frame);
					if (ret < 0)
						continue;
					av_init_packet(&enc_pkt);
					enc_pkt.data = nullptr;
					enc_pkt.size = 0;

					while (avcodec_receive_packet(out_codec_ctx, &enc_pkt) == 0)
					{
						enc_pkt.stream_index = out_audio_stream->index;
						av_packet_rescale_ts(&enc_pkt, out_codec_ctx->time_base, out_audio_stream->time_base);
						ret = av_interleaved_write_frame(out_audio_ctx, &enc_pkt);
						av_packet_unref(&enc_pkt);
					}
				}
			}
		}
		av_packet_unref(&pkt);
	}
	av_write_trailer(out_audio_ctx);
	avcodec_free_context(&out_codec_ctx);
	av_frame_free(&in_frame);
	av_frame_free(&out_frame);
	swr_free(&swr_ctx);

	if (!(out_audio_ctx->oformat->flags & AVFMT_NOFILE))
	{
		avio_closep(&out_audio_ctx->pb);
	}
	av_dump_format(out_audio_ctx, 0, path, 1);
	avcodec_free_context(&in_codec_ctx);

	avformat_close_input(&out_audio_ctx);
	avformat_free_context(out_audio_ctx);

	avformat_close_input(&in_audio_ctx);
	avformat_free_context(in_audio_ctx);

	avformat_network_deinit();
	return 0;
}
