#include "VideoRender.h"
#include "VideoState.h"

VideoRender::VideoRender()
{
}

VideoRender::~VideoRender()
{
    stop();
    cleanup();
}

void VideoRender::set_video_state(VideoState* vs)
{
    vs_ = vs;
}

void VideoRender::start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_) return;

    auto codec_ctx = vs_->get_video_param().codec_ctx;
    win_ = SDL_CreateWindow("Real player by rust.lyu", SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, codec_ctx->width, codec_ctx->height, 
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!win_) return;

    renderer_ = SDL_CreateRenderer(win_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) return;

    texture_ = SDL_CreateTexture(renderer_,
        SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, codec_ctx->width, codec_ctx->height);
    if (!texture_) return;

    running_ = true;
    th_ = std::thread([this, codec_ctx]() {
        SDL_Event event;
        SDL_Rect rect;
        rect.x = 0;
        rect.y = 0;
        rect.w = codec_ctx->width;
        rect.h = codec_ctx->height;

        uint32_t frameStart;
        const int targetFrameTime = 1000 / 60; // 60 FPS

        while (running_)
        {
            frameStart = SDL_GetTicks();

            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                case SDL_QUIT:
                    running_ = false;
                    return;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_f) {
                        std::lock_guard<std::mutex> lock(mutex_);
                        fullscreen_ = !fullscreen_;
                        SDL_SetWindowFullscreen(win_, fullscreen_ ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                    }
                    break;
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        std::lock_guard<std::mutex> lock(mutex_);
                        rect.w = event.window.data1;
                        rect.h = event.window.data2;
                    }
                    break;
                }
            }

            auto frameYUV = vs_->get_video_frame();
            double frame_pts = frameYUV.pts * av_q2d(codec_ctx->time_base);
            double audio_time = vs_->get_audio_clock();
            std::cout << "frame_pts:" << frame_pts << " audio_time:" << audio_time << std::endl;
            // 动态调整视频帧渲染
            if (!isnan(frame_pts)) {
                double diff = frame_pts - audio_time;
                if (diff > 0) {
                    // 等待到目标时间（留 10ms 余量避免过度等待）
                    double wait_time = diff - 0.01;
                    if (wait_time > 0) {
                        double wait_time = diff - 0.01;
                        std::cout << "diff:" << diff << std::endl;
                        //auto wait_duration = std::chrono::duration<double>(wait_time);
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                }
                else if (diff < -0.1) {
                    // 丢弃落后过多的帧
                    //av_frame_unref(&frameYUV);
                    continue;
                }
            }

            SDL_UpdateYUVTexture(texture_, nullptr, frameYUV.data[0], frameYUV.linesize[0],
                frameYUV.data[1], frameYUV.linesize[1], frameYUV.data[2], frameYUV.linesize[2]);

            SDL_RenderClear(renderer_);
            SDL_RenderCopy(renderer_, texture_, nullptr, &rect);
            SDL_RenderPresent(renderer_);

            int frameTime = SDL_GetTicks() - frameStart;
            if (frameTime < targetFrameTime) {
                SDL_Delay(targetFrameTime - frameTime);
            }
        }
    });
}

void VideoRender::stop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!running_) return;

    running_ = false;
    if (th_.joinable()) {
        th_.join();
    }
}

void VideoRender::cleanup()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (win_) {
        SDL_DestroyWindow(win_);
        win_ = nullptr;
    }
}
