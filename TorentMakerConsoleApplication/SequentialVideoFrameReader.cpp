#include "SequentialVideoFrameReader.h"
#include "Helpers\FFmpegHelpers.h"

#include <iostream>
#include <libhelpers\ScopedValue.h>
#include <libhelpers\unique_ptr_extensions.h>

SequentialVideoFrameReader::SequentialVideoFrameReader(const std::wstring &path)
	: ioCtx(path), duration(AV_NOPTS_VALUE), seekFlags(0), seekStreamIdx(-1),
	videoStreamIdx(-1), progress(0), frameDirty(true), latestPts(0)
{
	int ffmpegRes = 0;
	auto tmpNameUtf8 = H::Text::ConvertToUTF8(L"anykey" + this->ioCtx.GetExtension());

	MakeUnique(this->fmtCtx, avformat_alloc_context());
	this->fmtCtx->pb = this->ioCtx.GetAvioCtx();

	ffmpegRes = avformat_open_input(GetAddressOf(this->fmtCtx), tmpNameUtf8.c_str(), nullptr, nullptr);
	FFmpegHelpers::ThrowIfFFmpegFailed(ffmpegRes);

	ffmpegRes = avformat_find_stream_info(this->fmtCtx.get(), nullptr);
	FFmpegHelpers::ThrowIfFFmpegFailed(ffmpegRes);

	av_format_inject_global_side_data(this->fmtCtx.get());

	AVCodec *dec = nullptr;
	this->videoStreamIdx = av_find_best_stream(this->fmtCtx.get(), AVMediaType::AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
	FFmpegHelpers::ThrowIfFFmpegFailed(this->videoStreamIdx);

	AVStream *stream = this->fmtCtx->streams[this->videoStreamIdx];

	for (int i = 0; i < (int)this->fmtCtx->nb_streams; i++) {
		auto stream = this->fmtCtx->streams[i];

		if (i != this->videoStreamIdx) {
			stream->discard = AVDISCARD_ALL;
		}
		else {
			stream->discard = AVDISCARD_DEFAULT;

			MakeUnique(this->decCtx, stream->codec);
			ffmpegRes = avcodec_open2(this->decCtx.get(), dec, nullptr);
			FFmpegHelpers::ThrowIfFFmpegFailed(ffmpegRes);
		}
	}

	MakeUnique(this->frame, av_frame_alloc());

	// TODO add stream_time support for seeking
	if (this->fmtCtx->duration < 0) {
		if (stream->duration < 0) {
			this->duration = avio_size(fmtCtx->pb);
			this->durationUnits.den = durationUnits.num = 1;
			this->streamUnits.den = streamUnits.num = 1;
			this->seekFlags |= AVSEEK_FLAG_BYTE;
		}
		else {
			this->duration = stream->duration;
			this->durationUnits = stream->time_base;
			this->streamUnits = stream->time_base;
			this->seekStreamIdx = this->videoStreamIdx;
		}
	}
	else {
		this->duration = fmtCtx->duration;
		this->durationUnits = FFmpegHelpers::AVTimeBaseQ;
		this->streamUnits = stream->time_base;
	}
}

//SequentialVideoFrameReader::SequentialVideoFrameReader(SequentialVideoFrameReader &&other) {
//
//}

SequentialVideoFrameReader::~SequentialVideoFrameReader() {
}

//SequentialVideoFrameReader &SequentialVideoFrameReader::operator=(SequentialVideoFrameReader &&other) {
//
//}

uint64_t SequentialVideoFrameReader::GetProgress() const {
	return this->progress;
}

uint64_t SequentialVideoFrameReader::GetProgressDelta() const {
	uint64_t delta = (uint64_t)this->duration;
	return delta;
}

void SequentialVideoFrameReader::IncrementProgress(uint64_t v) {
	this->progress =
		H::Math::Clamp(this->progress + v, 0ULL, this->GetProgressDelta());
	this->frameDirty = true;
}

AVFrame *SequentialVideoFrameReader::GetFrame() {
	if (this->frameDirty) {
		this->frameDirty = false;

		int ffmpegRes = 0;
		int64_t framePts = 0;
		int64_t curPts = 0;
		int64_t time = (int64_t)this->progress;
		int64_t dstPts = av_rescale_q_rnd(
			time, 
			this->durationUnits, 
			this->streamUnits, 
			AV_ROUND_UP);
		int droppedFrames = 0;

		if (!(this->seekFlags & AVSEEK_FLAG_BYTE)) {
			avcodec_flush_buffers(this->decCtx.get());
			ffmpegRes = av_seek_frame(
				this->fmtCtx.get(), 
				this->seekStreamIdx, 
				time, 
				this->seekFlags);
			FFmpegHelpers::ThrowIfFFmpegFailed(ffmpegRes);

			/*if (FFmpegHelpers::IsFFmpegFailed(ffmpegRes)) {
			std::cout << std::endl;
			std::cout << "WARNING: av_seek_frame failed. Trying to read something may be slow :( or even crash x(";
			std::cout << std::endl;
			}*/
		}

		while (curPts < dstPts) {
			int gotFrame = 0;
			auto pkt = FFmpegHelpers::MakePacket(); // very important to use new pkt for each av_read_frame to avoid memory leak

			if (av_read_frame(this->fmtCtx.get(), &pkt) >= 0) {
				auto origPktScoped = MakeScopedValue(pkt, [](AVPacket *v) { av_packet_unref(v); });

				if (pkt.pts != AV_NOPTS_VALUE) {
					this->latestPts = pkt.pts;
				}

				if (this->seekFlags & AVSEEK_FLAG_BYTE && pkt.pos < dstPts) {
					continue;
				}

				if (pkt.stream_index == this->videoStreamIdx) {
					do {
						int ret = 0;
						{
							ret = avcodec_decode_video2(this->decCtx.get(), this->frame.get(), &gotFrame, &pkt);

							if (gotFrame) {
								framePts = av_frame_get_best_effort_timestamp(this->frame.get());

								if (this->seekFlags & AVSEEK_FLAG_BYTE) {
									curPts = pkt.pos;
								}
								else {
									curPts = framePts;
								}

								if (curPts < dstPts) {
									droppedFrames++;
								}
								else {
									std::cout << "Dropped frames: " << droppedFrames << std::endl;
									std::cout << "Frame pts: " << curPts << " Target pts: " << dstPts << std::endl;
								}
							}
						}

						if (ret < 0) {
							break;
						}

						pkt.data += ret;
						pkt.size -= ret;
					} while (pkt.size > 0);
				}// if (pkt.stream_index == this->videoStreamIdx)
			} // if (av_read_frame(this->fmtCtx.get(), &pkt) >= 0)
		} // while (curPts < dstPts)
	} // if (this->frameDirty)

	return this->frame.get();
}

VideoInfo SequentialVideoFrameReader::End() {
	VideoInfo info;

	if (this->seekFlags & AVSEEK_FLAG_BYTE) {
		int readResult = 0;

		while (readResult >= 0) {
			auto pkt = FFmpegHelpers::MakePacket();

			readResult = av_read_frame(this->fmtCtx.get(), &pkt);

			if (readResult >= 0) {
				auto origPktScoped = MakeScopedValue(pkt, [](AVPacket *v) { av_packet_unref(v); });

				if (pkt.stream_index == this->videoStreamIdx && pkt.pts != AV_NOPTS_VALUE) {
					this->latestPts = pkt.pts;
				}
			}
		}

		auto stream = this->fmtCtx->streams[this->videoStreamIdx];

		info.duration = latestPts;
		info.durationUnits = stream->time_base;
	}
	else {
		info.duration = this->duration;
		info.durationUnits = this->durationUnits;
	}

	return info;
}