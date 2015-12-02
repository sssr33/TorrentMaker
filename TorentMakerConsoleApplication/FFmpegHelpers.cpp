#include "FFmpegHelpers.h"

AVPacket FFmpegHelpers::MakePacket() {
	AVPacket pkt;

	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;

	return pkt;
}