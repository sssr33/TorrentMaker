#pragma once
#include "FFmpegIoCtx.h"
#include "Helpers\Deleters.h"

#include <memory>
#include <libhelpers\H.h>

extern "C" {
#include <libavformat\avformat.h>
}

struct VideoInfo {
	int64_t duration;
	AVRational durationUnits;
};

class SequentialVideoFrameReader {
public:
	NO_COPY(SequentialVideoFrameReader);

	SequentialVideoFrameReader(const std::wstring &path);
	SequentialVideoFrameReader(SequentialVideoFrameReader &&other);
	~SequentialVideoFrameReader();

	SequentialVideoFrameReader &operator=(SequentialVideoFrameReader &&other);

	uint64_t GetProgress() const;
	uint64_t GetProgressDelta() const;
	DirectX::XMUINT2 GetFrameSize() const;

	void IncrementProgress(uint64_t v);

	AVFrame *GetFrame();

	VideoInfo End();

private:
	FFmpegIoCtx ioCtx;
	std::unique_ptr<AVFormatContext, AVFormatContextDeleter> fmtCtx;
	std::unique_ptr<AVCodecContext, AVCodecContextDeleter> decCtx;
	std::unique_ptr<AVFrame, AVFrameDeleter> frame;

	int64_t duration;
	AVRational durationUnits, streamUnits;
	int seekFlags;
	int seekStreamIdx;
	int videoStreamIdx;
	uint64_t progress;
	bool frameDirty;
	int64_t latestPts;
};