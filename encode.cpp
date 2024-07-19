#include "encode.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

int FFmpegStudyEncode::encode(const char* path)
{
	uint8_t endcode[] = { 0, 0, 1, 0xb7 };
	const AVCodec* codec = avcodec_find_encoder_by_name("libx265");

	AVPacket* pkt = av_packet_alloc();
	//av_init_packet(&pkt);
	pkt->data = nullptr;
	pkt->size = 0;

	AVCodecContext* ctx = avcodec_alloc_context3(codec);
	ctx->bit_rate = 4000000;
	ctx->width = 1920;
	ctx->height = 1080;
	ctx->time_base = { 1, 60 };
	ctx->framerate = { 60, 1};
	ctx->gop_size = 10;
	ctx->max_b_frames = 1;
	ctx->pix_fmt = AV_PIX_FMT_YUV420P;
	if (codec->id == AV_CODEC_ID_H264)
	{
		av_opt_set(ctx->priv_data,"preset", "slow", 0);
	}

	if (avcodec_open2(ctx, codec, NULL) < 0)
		return -1;

	auto f = fopen(path, "wb");
	if (f == nullptr)
		return -1;
	AVFrame* frame = av_frame_alloc();
	frame->format = ctx->pix_fmt;
	frame->width = ctx->width;
	frame->height = ctx->height;
	if (av_frame_get_buffer(frame, 0) < 0)
	{
		return -1;
	}

	// yuv420
	for (int i = 0; i < 60; ++i)
	{
		fflush(stdout);
		if (av_frame_make_writable(frame) < 0)
			return -1;

		// write Y
		for (int y = 0; y < frame->height; ++y)
		{
			for (int x = 0; x < frame->width; ++x)
			{
				frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
			}
		}

		// wirte cb&cr
		for (int y = 0; y < frame->height/2; ++y)
		{
			for (int x = 0; x < frame->width/2; ++x)
			{
				frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;// cb
				frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5; // cr
			}
		}
		frame->pts = i;
		write_to_file(ctx, frame, pkt, f);
	}
	write_to_file(ctx, NULL, pkt, f);

	if (codec->id == AV_CODEC_ID_MPEG1VIDEO || codec->id == AV_CODEC_ID_MPEG2VIDEO)
		fwrite(endcode, 1, sizeof(endcode), f);
	fclose(f);
	avcodec_free_context(&ctx);
	av_frame_free(&frame);
	av_packet_free(&pkt);
	return 0;
}

int FFmpegStudyEncode::write_to_file(AVCodecContext* ctx, AVFrame* frame, AVPacket* pkt, FILE* file)
{
	int ret = avcodec_send_frame(ctx, frame);
	if ( ret < 0)
		return -1;
	while (ret >= 0)
	{
		ret = avcodec_receive_packet(ctx, pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return -1;
		else if (ret < 0)
			return -2;
		fwrite(pkt->data, 1, pkt->size, file);
		av_packet_unref(pkt);
	}
	return 0;
}
