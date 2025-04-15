#include "VideoRender.h"
#include "VideoState.h"

#include <SDL.h>

VideoRender::VideoRender()
{
}

VideoRender::~VideoRender()
{
}

void VideoRender::set_video_state(VideoState* vs)
{
	vs_ = vs;
}

void VideoRender::start()
{
	SDL_Window* win = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_Texture* texture = nullptr;
	auto codec_ctx = vs_->get_video_param().codec_ctx;
	win = SDL_CreateWindow("test", SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED, codec_ctx->width, codec_ctx->height, SDL_WINDOW_OPENGL);
	renderer = SDL_CreateRenderer(win, -1, 0);
	texture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, codec_ctx->width, codec_ctx->height);
	SDL_Event event;
	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = codec_ctx->width;
	rect.h = codec_ctx->height;

	th_ = std::thread([&]() {
		while (1)
		{
			auto frameYUV = vs_->get_video_frame();
			SDL_UpdateYUVTexture(texture, &rect, frameYUV.data[0], frameYUV.linesize[0],
				frameYUV.data[1], frameYUV.linesize[1], frameYUV.data[2], frameYUV.linesize[2]);

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
			SDL_Delay(40);
		}
		});
	th_.detach();
}
