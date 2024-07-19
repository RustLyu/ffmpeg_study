#ifndef DECODE_H
#define DECODE_H

#include <tuple>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;

namespace FFmpegStudy{
	AVFormatContext* open_file(const char* path);
	std::tuple<int, int> get_audio_vedio_index(AVFormatContext* ctx);
	int save_fame_as_jpeg(AVFrame* frame, AVCodecContext* ctx, int no);
	int decode(AVFormatContext* ctx, int video_index);
	void close(AVFormatContext* ctx);
}
#endif DECODE_H
