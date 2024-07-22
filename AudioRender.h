#ifndef AUDIO_RENDER_H
#define AUDIO_RENDER_H

#include <thread>

class VideoState;

class AudioRender {
public:
	AudioRender();
	~AudioRender();

public:
	int start();
	void set_video_state(VideoState* vs);

private:
	VideoState* vs_;
};

#endif //AUDIO_RENDER_H
