// TorentMakerConsoleApplication.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "FileDialogEventsImpl.h"
#include "FFmpegIoCtx.h"
#include "ScopedValue.h"
#include "FFmpegHelpers.h"
#include "DxHelpres\DxDevice.h"

#include <iostream>
#include <Windows.h>
#include <wrl.h>
#include <ShObjIdl.h>
#include <libhelpers\H.h>
#include <libhelpers\CoUniquePtr.h>

extern "C" {
#include <libavformat\avformat.h>
}

template<class T, class D>
class GetAddressOfUnique {
public:
	GetAddressOfUnique(std::unique_ptr<T, D> &ptr)
		: ptr(ptr)
	{
		this->rawPtr = this->ptr.release();
	}

	operator T **() {
		return &this->rawPtr;
	}

	~GetAddressOfUnique() {
		this->ptr.reset(this->rawPtr);
	}

private:
	T *rawPtr;
	std::unique_ptr<T, D> &ptr;
};

template<class T, class D>
GetAddressOfUnique<T, D> GetAddressOf(std::unique_ptr<T, D> &v) {
	return GetAddressOfUnique<T, D>(v);
}

template<class T, class D>
std::unique_ptr<T, D> WrapUnique(T *ptr, const D &deleter) {
	return std::unique_ptr<T, D>(ptr, deleter);
}

void EnumFiles(
	const std::wstring &folder,
	const std::function<void(const std::wstring &file)> &onFile);

void CheckFFmpegResult(int ffmpegRes);

#define AV_TIME_BASE_Q_CPP { 1, AV_TIME_BASE }

int main() {
	HRESULT hr = S_OK;

	av_register_all();
	hr = CoInitialize(nullptr);
	H::System::ThrowIfFailed(hr);

	Microsoft::WRL::ComPtr<IFileDialog> fileDialog;
	hr = CoCreateInstance(CLSID_FileOpenDialog,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(fileDialog.GetAddressOf()));
	H::System::ThrowIfFailed(hr);

	Microsoft::WRL::ComPtr<FileDialogEventsImpl> fileDialogEvents = Microsoft::WRL::Make<FileDialogEventsImpl>();

	DWORD dwCookie;
	hr = fileDialog->Advise(fileDialogEvents.Get(), &dwCookie);
	H::System::ThrowIfFailed(hr);

	DWORD dwFlags;
	hr = fileDialog->GetOptions(&dwFlags);
	H::System::ThrowIfFailed(hr);

	hr = fileDialog->SetOptions(dwFlags | FOS_PICKFOLDERS);
	H::System::ThrowIfFailed(hr);

	// Set the default extension to be ".doc" file.
	/*hr = fileDialog->SetDefaultExtension(L"doc;docx");
	H::System::ThrowIfFailed(hr);*/

	// Show the dialog
	hr = fileDialog->Show(NULL);

	if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
		// folder selection is cancelled so just exit
		// TODO add some message
		return 0;
	}

	H::System::ThrowIfFailed(hr);

	// Obtain the result once the user clicks 
	// the 'Open' button.
	// The result is an IShellItem object.
	Microsoft::WRL::ComPtr<IShellItem> psiResult;
	hr = fileDialog->GetResult(psiResult.GetAddressOf());
	H::System::ThrowIfFailed(hr);

	std::wstring path;

	{
		CoUniquePtr<WCHAR> pathTmp;
		psiResult->GetDisplayName(SIGDN_FILESYSPATH, pathTmp.GetAddressOf());
		path = pathTmp.get();
	}

	// TODO remove while when code will be stable
	while (true) {
		EnumFiles(path, [&](const std::wstring &tmp) {
			std::wcout << L"File: " << tmp << std::endl;

			// TODO test memleak on .zip or another unsupported files.

			int ffmpegRes = 0;
			FFmpegIoCtx ioCtx(tmp);

			auto fmtCtx = WrapUnique(avformat_alloc_context(), [](AVFormatContext *v) { avformat_close_input(&v); });
			fmtCtx->pb = ioCtx.GetAvioCtx();

			auto tmpNameUtf8 = H::Text::ConvertToUTF8(L"anykey" + ioCtx.GetExtension());

			ffmpegRes = avformat_open_input(GetAddressOf(fmtCtx), tmpNameUtf8.c_str(), nullptr, nullptr);
			CheckFFmpegResult(ffmpegRes);

			ffmpegRes = avformat_find_stream_info(fmtCtx.get(), nullptr);
			CheckFFmpegResult(ffmpegRes);

			av_format_inject_global_side_data(fmtCtx.get());

			AVCodec *dec = nullptr;
			auto decCtx = WrapUnique((AVCodecContext *)nullptr, [](AVCodecContext *v) { avcodec_close(v); });
			auto videoStreamIdx = av_find_best_stream(fmtCtx.get(), AVMediaType::AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
			CheckFFmpegResult(videoStreamIdx);

			AVStream *stream = fmtCtx->streams[videoStreamIdx];

			for (int i = 0; i < (int)fmtCtx->nb_streams; i++) {
				auto stream = fmtCtx->streams[i];

				if (i != videoStreamIdx) {
					stream->discard = AVDISCARD_ALL;
				}
				else {
					stream->discard = AVDISCARD_DEFAULT;

					decCtx.reset(stream->codec);
					ffmpegRes = avcodec_open2(decCtx.get(), dec, nullptr);
					CheckFFmpegResult(ffmpegRes);
				}
			}

			int photoCount = 10;
			int64_t duration = AV_NOPTS_VALUE;
			AVRational durationUnits, streamUnits;
			int seekFlags = 0;
			int seekStreamIdx = -1;

			auto frame = WrapUnique(av_frame_alloc(), [](AVFrame *v) { av_frame_free(&v); });
			int64_t latestPts = 0;

			// TODO add stream_time support for seeking
			if (fmtCtx->duration < 0) {
				if (stream->duration < 0) {
					duration = avio_size(fmtCtx->pb);
					durationUnits.den = durationUnits.num = 1;
					streamUnits.den = streamUnits.num = 1;
					seekFlags |= AVSEEK_FLAG_BYTE;
				}
				else {
					duration = stream->duration;
					durationUnits = stream->time_base;
					streamUnits = stream->time_base;
					seekStreamIdx = videoStreamIdx;
				}

				return;
			}
			else {
				duration = fmtCtx->duration;
				durationUnits = AV_TIME_BASE_Q_CPP;
				streamUnits = stream->time_base;
			}

			int64_t timeStep = duration / (photoCount + 2); // +2 for begin, end
			int64_t endTime = duration - timeStep;

			for (int64_t time = timeStep; time < endTime; time += timeStep) {
				int64_t framePts = 0;
				int64_t curPts = 0;
				int64_t dstPts = av_rescale_q_rnd(time, durationUnits, streamUnits, AV_ROUND_UP);
				int droppedFrames = 0;

				if (!(seekFlags & AVSEEK_FLAG_BYTE)) {
					avcodec_flush_buffers(decCtx.get());
					ffmpegRes = av_seek_frame(fmtCtx.get(), seekStreamIdx, time, seekFlags);
					CheckFFmpegResult(ffmpegRes);
				}

				while (curPts < dstPts) {
					int gotFrame = 0;
					auto pkt = FFmpegHelpers::MakePacket(); // very important to use new pkt for each av_read_frame to avoid memory leak

					if (av_read_frame(fmtCtx.get(), &pkt) >= 0) {
						auto origPktScoped = MakeScopedValue(pkt, [](AVPacket *v) { av_packet_unref(v); });

						if (pkt.pts != AV_NOPTS_VALUE) {
							latestPts = pkt.pts;
						}

						if (seekFlags & AVSEEK_FLAG_BYTE && pkt.pos < dstPts) {
							continue;
						}

						if (pkt.stream_index == videoStreamIdx) {
							do {
								int ret = 0;
								{
									ret = avcodec_decode_video2(decCtx.get(), frame.get(), &gotFrame, &pkt);

									if (gotFrame) {
										framePts = av_frame_get_best_effort_timestamp(frame.get());

										if (seekFlags & AVSEEK_FLAG_BYTE) {
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
						}
					}
				}

				DxDevice dxDev;
			}

			if (seekFlags & AVSEEK_FLAG_BYTE) {
				int readResult = 0;

				while (readResult >= 0) {
					auto pkt = FFmpegHelpers::MakePacket();

					readResult = av_read_frame(fmtCtx.get(), &pkt);

					if (readResult >= 0) {
						auto origPktScoped = MakeScopedValue(pkt, [](AVPacket *v) { av_packet_unref(v); });

						if (pkt.stream_index == videoStreamIdx && pkt.pts != AV_NOPTS_VALUE) {
							latestPts = pkt.pts;
						}
					}
				}

				duration = latestPts;
				durationUnits = stream->time_base;
			}

			std::cout << std::endl;
		});
	}

	return 0;
}

void EnumFiles(
	const std::wstring &folder,
	const std::function<void(const std::wstring &file)> &onFile)
{
	WIN32_FIND_DATAW findData;
	auto tmpFolder = folder + L"\\*";
	auto find = MakeScopedValue(FindFirstFileW(tmpFolder.c_str(), &findData), [](HANDLE *v) { FindClose(*v); });

	do {
		bool isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		bool badName1 = findData.cFileName[0] == '.' && findData.cFileName[1] == '\0';
		bool badName2 = findData.cFileName[0] == '.' && findData.cFileName[1] == '.' && findData.cFileName[2] == '\0';

		if (!isDir || (!badName1 && !badName2))
		{
			auto itemPath = folder + L'\\' + findData.cFileName;

			if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				EnumFiles(itemPath, onFile);
			}
			else {
				try {
					onFile(itemPath);
				}
				catch (...) {
					std::cout << "Failed to process ";
					std::wcout << itemPath;
					std::cout << " file." << std::endl;
				}
			}
		}
	} while (FindNextFileW(find.GetRef(), &findData));
}

void CheckFFmpegResult(int ffmpegRes) {
	if (ffmpegRes < 0) {
		throw std::exception();
	}
}