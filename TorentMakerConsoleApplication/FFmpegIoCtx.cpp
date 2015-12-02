#include "FFmpegIoCtx.h"

#include <libhelpers\H.h>

FFmpegIoCtx::FFmpegIoCtx(const std::wstring &path, int bufSize)
	: path(path), ctx(nullptr), file(INVALID_HANDLE_VALUE)
{
	auto buf = static_cast<uint8_t *>(av_malloc((size_t)bufSize));
	this->ctx = avio_alloc_context(
		buf, bufSize,
		0, 
		this, 
		FFmpegIoCtx::read_packet, 
		nullptr, 
		FFmpegIoCtx::seek);

	this->file = CreateFileW(
		path.c_str(), 
		GENERIC_READ, FILE_SHARE_READ, 
		nullptr, 
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 
		nullptr);
}

FFmpegIoCtx::~FFmpegIoCtx() {
	if (this->file != INVALID_HANDLE_VALUE) {
		CloseHandle(this->file);
	}

	if (this->ctx) {
		/* It may be freed and replaced with a new buffer by libavformat.
		*  AVIOContext.buffer holds the buffer currently in use,
		*  which must be later freed with av_free().
		*/
		av_free(this->ctx->buffer);
		av_free(this->ctx);
	}
}

int64_t FFmpegIoCtx::GetFileSize() const {
	LARGE_INTEGER size;

	if (!GetFileSizeEx(this->file, &size)) {
		size.QuadPart = -1;
	}

	return size.QuadPart;
}

AVIOContext *FFmpegIoCtx::GetAvioCtx() const {
	return this->ctx;
}

std::wstring FFmpegIoCtx::GetExtension() const {
	std::wstring extension;
	auto offset = this->path.find_last_of(L'.');

	if (offset != std::wstring::npos) {
		extension = this->path.substr(offset);
	}

	return extension;
}

int FFmpegIoCtx::read_packet(void *opaque, uint8_t *buf, int buf_size) {
	auto _this = (FFmpegIoCtx *)opaque;
	DWORD toRead = (DWORD)buf_size, readed = 0;

	if (!ReadFile(_this->file, buf, toRead, &readed, nullptr)) {
		readed = -1;
	}

	return (int)readed;
}

int64_t FFmpegIoCtx::seek(void *opaque, int64_t offset, int whence) {
	auto _this = (FFmpegIoCtx *)opaque;
	LARGE_INTEGER distToMove, res;
	DWORD moveMethod = -1;

	switch (whence) {
	case SEEK_SET: {
		distToMove.QuadPart = offset;
		moveMethod = FILE_BEGIN;
		break;
	}
	case SEEK_CUR: {
		distToMove.QuadPart = offset;
		moveMethod = FILE_CURRENT;
		break;
	}
	case SEEK_END: {
		distToMove.QuadPart = offset;
		moveMethod = FILE_END;
		break;
	}
	case AVSEEK_SIZE: {
		res.QuadPart = _this->GetFileSize();
		break;
	}
	default:
		res.QuadPart = -1;
		break;
	}

	if (moveMethod != -1) {
		if (!SetFilePointerEx(_this->file, distToMove, &res, moveMethod)) {
			res.QuadPart = -1;
		}
	}

	return res.QuadPart;
}