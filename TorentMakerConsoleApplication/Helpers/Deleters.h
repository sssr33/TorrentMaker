#pragma once

#include <libhelpers\H.h>

extern "C" {
#include <libavformat\avformat.h>
}

struct FindHandleDeleter {
	void operator()(HANDLE *v);
};

struct AVFormatContextDeleter {
	void operator()(AVFormatContext *v);
};

struct AVCodecContextDeleter {
	void operator()(AVCodecContext *v);
};

struct AVFrameDeleter {
	void operator()(AVFrame *v);
};