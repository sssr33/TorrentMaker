#include "FFmpegHelpers.h"

const AVRational FFmpegHelpers::AVTimeBaseQ = { 1, AV_TIME_BASE };

bool FFmpegHelpers::IsFFmpegFailed(int ffmpegRes) {
	bool failed = ffmpegRes < 0;
	return failed;
}

void FFmpegHelpers::ThrowIfFFmpegFailed(int ffmpegRes) {
	if (FFmpegHelpers::IsFFmpegFailed(ffmpegRes)) {
		throw std::exception();
	}
}

AVPacket FFmpegHelpers::MakePacket() {
	AVPacket pkt;

	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;

	return pkt;
}