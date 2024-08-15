#ifndef GRAB_AUDIO_H
#define GRAB_AUDIO_H

namespace FFmpeg_Grab_Audio {

	class Grab {
	public:
		static int grab(const char* path);
		static bool stop_;
	};
}

#endif
