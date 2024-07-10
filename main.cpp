#include <iostream>
#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

void print_video_info(const std::string& filename) {
    // 初始化FFmpeg库
    avformat_network_init();

    // 打开视频文件
    AVFormatContext* formatContext = avformat_alloc_context();
    if (avformat_open_input(&formatContext, filename.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return;
    }

    // 读取视频文件信息
    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        std::cerr << "Could not retrieve stream info from file: " << filename << std::endl;
        avformat_close_input(&formatContext);
        return;
    }

    // 打印视频文件信息
    av_dump_format(formatContext, 0, filename.c_str(), 0);

    // 清理
    avformat_close_input(&formatContext);
    avformat_free_context(formatContext);
    avformat_network_deinit();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <video file>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    print_video_info(filename);

    return 0;
}

