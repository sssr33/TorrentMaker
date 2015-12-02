#pragma once

extern "C" {
#include <libavformat\avformat.h>
}

class FFmpegHelpers {
public:
	static AVPacket MakePacket();
};