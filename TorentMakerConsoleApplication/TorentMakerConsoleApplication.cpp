// TorentMakerConsoleApplication.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "FileDialogEventsImpl.h"
#include "FFmpegIoCtx.h"
#include "ScopedValue.h"
#include "FFmpegHelpers.h"
#include "DxHelpres\DxDevice.h"
#include "Texture\Yuv420pTexture.h"
#include "Texture\Bgra8CopyTexture.h"
#include "Texture\Bgra8RenderTarget.h"

#include <iostream>
#include <Windows.h>
#include <wrl.h>
#include <ShObjIdl.h>
#include <libhelpers\H.h>
#include <libhelpers\CoUniquePtr.h>
#include <libhelpers\ImageUtils.h>

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

template<uint32_t size>
std::array<D3D11_SUBRESOURCE_DATA, size> GetData(const AVFrame *frame) {
	std::array<D3D11_SUBRESOURCE_DATA, size> data;

	for (uint32_t i = 0; i < size; i++) {
		data[i].pSysMem = frame->data[i];
		data[i].SysMemPitch = frame->linesize[i];
		data[i].SysMemSlicePitch = 0;
	}

	return data;
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
				auto d3dDev = dxDev.GetD3DDevice();
				auto d3dCtx = dxDev.GetD3DContext();

				Yuv420pTexture<D3D11_USAGE_IMMUTABLE> yuvTex(
					d3dDev, 
					DirectX::XMUINT2((uint32_t)frame->width, (uint32_t)frame->height),
					GetData<3>(frame.get()));

				Bgra8RenderTarget bgraTex(
					d3dDev,
					DirectX::XMUINT2((uint32_t)frame->width, (uint32_t)frame->height),
					0);
				Bgra8CopyTexture bgraTexCopy(
					d3dDev,
					DirectX::XMUINT2((uint32_t)frame->width, (uint32_t)frame->height));

				Microsoft::WRL::ComPtr<ID3D11Buffer> fltIdxBuf;

				Microsoft::WRL::ComPtr<ID3D11InputLayout> fltIdxInputLayout;
				Microsoft::WRL::ComPtr<ID3D11Buffer> fltIdxCBuffer;
				Microsoft::WRL::ComPtr<ID3D11VertexShader> fltIdxVs;

				Microsoft::WRL::ComPtr<ID3D11PixelShader> yuv420pToRgbaPs;

				Microsoft::WRL::ComPtr<ID3D11SamplerState> pointSampler;
				Microsoft::WRL::ComPtr<ID3D11SamplerState> linearSampler;

				//flt idx buffer
				{
					float idx[] = { 0, 1, 2, 3 };
					D3D11_BUFFER_DESC bufDesc;
					D3D11_SUBRESOURCE_DATA subResData;

					bufDesc.ByteWidth = sizeof(idx);
					bufDesc.Usage = D3D11_USAGE_IMMUTABLE;
					bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
					bufDesc.CPUAccessFlags = 0;
					bufDesc.MiscFlags = 0;
					bufDesc.StructureByteStride = 0;

					subResData.pSysMem = idx;
					subResData.SysMemPitch = 0;
					subResData.SysMemSlicePitch = 0;

					hr = d3dDev->CreateBuffer(&bufDesc, &subResData, fltIdxBuf.GetAddressOf());
					H::System::ThrowIfFailed(hr);

					bufDesc.ByteWidth = sizeof(DirectX::XMMATRIX);
					bufDesc.Usage = D3D11_USAGE_DEFAULT;
					bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

					hr = d3dDev->CreateBuffer(&bufDesc, &subResData, fltIdxCBuffer.GetAddressOf());
					H::System::ThrowIfFailed(hr);
				}

				// fltIdxVs
				{
					auto data = H::System::LoadPackageFile(L"QuadStripFromFltIndexVs.cso");

					hr = d3dDev->CreateVertexShader(data.data(), data.size(), nullptr, fltIdxVs.GetAddressOf());
					H::System::ThrowIfFailed(hr);

					D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
						{ "POSITION", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
					};

					hr = d3dDev->CreateInputLayout(inputDesc, ARRAY_SIZE(inputDesc), data.data(), data.size(), fltIdxInputLayout.GetAddressOf());
					H::System::ThrowIfFailed(hr);
				}

				// yuv420pToRgbaPs
				{
					auto data = H::System::LoadPackageFile(L"Yuv420pToRgbaPS.cso");

					hr = d3dDev->CreatePixelShader(data.data(), data.size(), nullptr, yuv420pToRgbaPs.GetAddressOf());
					H::System::ThrowIfFailed(hr);
				}

				// samplers
				{
					D3D11_SAMPLER_DESC samplerDesc;

					samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
					samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
					samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
					samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
					samplerDesc.MinLOD = -FLT_MAX;
					samplerDesc.MaxLOD = FLT_MAX;
					samplerDesc.MipLODBias = 0.0f;
					samplerDesc.MaxAnisotropy = 1;
					samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
					std::memset(samplerDesc.BorderColor, 0, sizeof(samplerDesc.BorderColor));

					hr = d3dDev->CreateSamplerState(&samplerDesc, pointSampler.GetAddressOf());
					H::System::ThrowIfFailed(hr);

					samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

					hr = d3dDev->CreateSamplerState(&samplerDesc, linearSampler.GetAddressOf());
					H::System::ThrowIfFailed(hr);
				}

				DirectX::XMMATRIX transform = DirectX::XMMatrixIdentity();

				transform = DirectX::XMMatrixMultiply(transform, DirectX::XMMatrixScaling(2, 2, 1));
				transform = DirectX::XMMatrixMultiply(transform, DirectX::XMMatrixTranslation(0, 0, 1));
				transform = DirectX::XMMatrixTranspose(transform);

				d3dCtx->UpdateSubresource(fltIdxCBuffer.Get(), 0, nullptr, &transform, 0, 0);

				bgraTex.Clear(d3dCtx, DirectX::Colors::White);
				bgraTex.SetRenderTarget(d3dCtx);
				bgraTex.SetViewport(d3dCtx);

				d3dCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

				{
					uint32_t stride = 4;
					uint32_t offset = 0;
					d3dCtx->IASetVertexBuffers(0, 1, fltIdxBuf.GetAddressOf(), &stride, &offset);
				}

				d3dCtx->IASetInputLayout(fltIdxInputLayout.Get());

				d3dCtx->VSSetConstantBuffers(0, 1, fltIdxCBuffer.GetAddressOf());
				d3dCtx->VSSetShader(fltIdxVs.Get(), nullptr, 0);

				d3dCtx->PSSetShader(yuv420pToRgbaPs.Get(), nullptr, 0);
				d3dCtx->PSSetSamplers(0, 1, pointSampler.GetAddressOf());
				yuvTex.SetToContextPS(d3dCtx, 0);

				d3dCtx->Draw(4, 0);

				bgraTexCopy.Copy(d3dCtx, &bgraTex);
				auto bgraTexCopyData = bgraTexCopy.Map(d3dCtx);
				ImageUtils imgUtils;

				auto savePath = H::System::GetPackagePath() + L"screen0.png";
				auto encoder = imgUtils.CreateEncoder(savePath, GUID_ContainerFormatPng);
				auto encodeFrame = imgUtils.CreateFrameForEncode(encoder.Get());

				imgUtils.EncodeAllocPixels(encodeFrame.Get(), DirectX::XMUINT2(decCtx->width, decCtx->height), GUID_WICPixelFormat32bppBGRA);
				imgUtils.EncodePixels(encodeFrame.Get(), decCtx->height, bgraTexCopyData.GetRowPitch(), bgraTexCopyData.GetRowPitch() * decCtx->height * 4, bgraTexCopyData.GetData());

				imgUtils.EncodeCommit(encodeFrame.Get());
				imgUtils.EncodeCommit(encoder.Get());
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