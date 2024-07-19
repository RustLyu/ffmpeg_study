#ifndef ENCODE_H
#define ENCODE_H

#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct AVCodecContext;
struct AVPacket;
struct AVFrame;

namespace FFmpegStudyEncode {
	int encode(const char* path);
	int write_to_file(AVCodecContext* ctx, AVFrame* frame, AVPacket* pkt, FILE* file);
}

#endif //ENCODE_H
