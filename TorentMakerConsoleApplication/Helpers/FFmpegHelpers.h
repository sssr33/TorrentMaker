#pragma once

#include <array>
#include <d3d11.h>

extern "C" {
#include <libavformat\avformat.h>
}

class FFmpegHelpers {
public:
	static const AVRational AVTimeBaseQ;

	static bool IsFFmpegFailed(int ffmpegRes);
	static void ThrowIfFFmpegFailed(int ffmpegRes);
	static AVPacket MakePacket();

	template<uint32_t size>
	static std::array<D3D11_SUBRESOURCE_DATA, size> GetData(const AVFrame *frame) {
		std::array<D3D11_SUBRESOURCE_DATA, size> data;

		for (uint32_t i = 0; i < size; i++) {
			data[i].pSysMem = frame->data[i];
			data[i].SysMemPitch = frame->linesize[i];
			data[i].SysMemSlicePitch = 0;
		}

		return data;
	}

};