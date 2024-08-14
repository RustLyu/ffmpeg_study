#include "GrabWindow.h"

#include <iostream>
#include <fstream>
#include "common.h"

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
bool FFmpegStudy_Grab::Grab::stop_ = false;
int FFmpegStudy_Grab::Grab::grab(const char* path)
{
	av_log_set_level(AV_LOG_TRACE);
	avdevice_register_all();
	avformat_network_init();
	/*******************in***********************************/
	AVFormatContext* ctx = avformat_alloc_context();
	if (ctx == nullptr)
		return -1;

	AVDictionary* options = NULL;

	char fpsChar[3];
	int m_fps = 30;
	itoa(m_fps, fpsChar, 10);
	av_dict_set(&options, "framerate", fpsChar, 0);

	const AVInputFormat* input_fmt = av_find_input_format("gdigrab");
	int ret = avformat_open_input(&ctx, "desktop", input_fmt, nullptr);
	if (ret < 0) {
		ERRROR_CODE_2_STR(ret);
		avformat_free_context(ctx);
		return -1;
	}

	if (avformat_find_stream_info(ctx, NULL) < 0)
	{
	}

	int video_index = -1;
	for (int i = 0; i < ctx->nb_streams; ++i)
	{
		if (ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			video_index = i;
		}
	}

	const AVCodec* in_video_codec = nullptr;
	if(video_index >= 0)
		in_video_codec = avcodec_find_decoder(ctx->streams[video_index]->codecpar->codec_id);

	AVCodecContext* in_codec_ctx = avcodec_alloc_context3(in_video_codec);

	if (avcodec_parameters_to_context(in_codec_ctx, ctx->streams[video_index]->codecpar) < 0)
		return -1;
	if (avcodec_open2(in_codec_ctx, in_video_codec, nullptr) < 0)
		return -1;



	AVFrame* frame = av_frame_alloc();

	frame->format = in_codec_ctx->pix_fmt;
	frame->width = in_codec_ctx->width;
	frame->height = in_codec_ctx->height;
	
	ret = av_image_alloc(frame->data, frame->linesize, frame->width, frame->height, in_codec_ctx->pix_fmt, 32);

	AVDictionary* avDict = NULL;

	/*******************out***********************************/
	AVFormatContext* output_ctx = nullptr;
	avformat_alloc_output_context2(&output_ctx, nullptr, nullptr, path);

	const AVCodec* out_put_video_codec = avcodec_find_encoder(AV_CODEC_ID_H264);

	AVStream* out_video_stream = avformat_new_stream(output_ctx, nullptr);

	AVCodecContext* out_codec_ctx = avcodec_alloc_context3(out_put_video_codec);

	out_codec_ctx->bit_rate = 4000*1000;
	out_codec_ctx->width = in_codec_ctx->width;
	out_codec_ctx->height = in_codec_ctx->height;
	out_codec_ctx->time_base = { 1, 30 };
	out_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

	if (output_ctx->oformat->flags & AVFMT_GLOBALHEADER)
		out_codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	AVDictionary* opt = NULL;
	av_dict_copy(&opt, avDict, 0);
	if (avcodec_open2(out_codec_ctx, out_put_video_codec, &opt) < 0)
		return -1;

	if (avcodec_parameters_from_context(out_video_stream->codecpar, out_codec_ctx) < 0)
		return -1;

	if (!(output_ctx->oformat->flags & AVFMT_NOFILE)) {
		if (avio_open(&output_ctx->pb, path, AVIO_FLAG_WRITE) < 0) {
			std::cerr << "Could not open output file" << std::endl;
			return -1;
		}
	}

	av_dump_format(output_ctx, 0, path, 1);
	if (avformat_write_header(output_ctx, nullptr) < 0) {
		std::cerr << "Could not write header" << std::endl;
		return -1;
	}


	SwsContext* sws = sws_getContext(in_codec_ctx->width,
		in_codec_ctx->height,
		in_codec_ctx->pix_fmt,
		out_codec_ctx->width,
		out_codec_ctx->height,
		out_codec_ctx->pix_fmt,
		SWS_BICUBIC, nullptr, nullptr, nullptr);
	if (sws == nullptr)
		return -1;

	//*********************process pkt************************************/
  
	AVFrame* out_frame = av_frame_alloc();
	out_frame->format = out_codec_ctx->pix_fmt;
	out_frame->width = out_codec_ctx->width;
	out_frame->height = out_codec_ctx->height;

	av_image_alloc(out_frame->data, out_frame->linesize, out_frame->width,
		out_frame->height, out_codec_ctx->pix_fmt, 32);

	int64_t last_pts = 0;
	AVPacket enc_pkt;
	while (!stop_)
	{
		AVPacket pkt;
		av_init_packet(&pkt);
		pkt.data = nullptr;
		pkt.size = 0;
		if (av_read_frame(ctx, &pkt) < 0)
		{
			break; 
		}
		if(pkt.stream_index == video_index)
		{
			ret = avcodec_send_packet(in_codec_ctx, &pkt);
			if (ret < 0)
			{
				ERRROR_CODE_2_STR(ret);
				av_packet_unref(&pkt);
				continue;
			}
			while (avcodec_receive_frame(in_codec_ctx, frame) == 0)
			{
				out_frame->pts = last_pts++;
				sws_scale(sws, frame->data, frame->linesize, 0, in_codec_ctx->height,
					out_frame->data, out_frame->linesize);
				ret = avcodec_send_frame(out_codec_ctx, out_frame);
				if (ret < 0) {
					ERRROR_CODE_2_STR(ret);
					std::cerr << "Error sending frame for encoding" << std::endl;
					continue;
				}
			
				av_init_packet(&enc_pkt);
				enc_pkt.data = nullptr;
				enc_pkt.size = 0;

				while (avcodec_receive_packet(out_codec_ctx, &enc_pkt) == 0) 
				{
					enc_pkt.stream_index = out_video_stream->index;
					av_packet_rescale_ts(&enc_pkt, out_codec_ctx->time_base, out_video_stream->time_base);
					ret = av_interleaved_write_frame(output_ctx, &enc_pkt);
					av_packet_unref(&enc_pkt);
				}
			}
		}
		av_packet_unref(&pkt);
	}
	av_write_trailer(output_ctx);

	avcodec_free_context(&out_codec_ctx);
	av_frame_free(&frame);
	av_frame_free(&out_frame);
	sws_freeContext(sws);

	if (!(output_ctx->oformat->flags & AVFMT_NOFILE))
	{
		avio_closep(&output_ctx->pb);
	}
	av_dump_format(output_ctx, 0, path, 1);
	avcodec_free_context(&in_codec_ctx);

	avformat_close_input(&output_ctx);
	avformat_free_context(output_ctx);
	
	avformat_close_input(&ctx);
	avformat_free_context(ctx);

	avformat_network_deinit();
	return 0;
}
