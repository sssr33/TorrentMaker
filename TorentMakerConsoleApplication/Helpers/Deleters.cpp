#include "Deleters.h"

void FindHandleDeleter::operator()(HANDLE *v) {
	FindClose(*v);
}

void AVFormatContextDeleter::operator()(AVFormatContext *v) {
	avformat_close_input(&v);
}

void AVCodecContextDeleter::operator()(AVCodecContext *v) {
	avcodec_close(v);
}

void AVFrameDeleter::operator()(AVFrame *v) {
	av_frame_free(&v);
}