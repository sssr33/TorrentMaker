#pragma once

#include <string>
#include <Windows.h>

extern "C" {
#include <libavformat\avformat.h>
}

class FFmpegIoCtx {
public:
	FFmpegIoCtx(const std::wstring &path, int bufSize = 1024 * 16);
	~FFmpegIoCtx();

	int64_t GetFileSize() const;
	AVIOContext *GetAvioCtx() const;
	std::wstring GetExtension() const;

private:
	AVIOContext *ctx;
	std::wstring path;
	HANDLE file;

	static int read_packet(void *opaque, uint8_t *buf, int buf_size);
	static int64_t seek(void *opaque, int64_t offset, int whence);
};