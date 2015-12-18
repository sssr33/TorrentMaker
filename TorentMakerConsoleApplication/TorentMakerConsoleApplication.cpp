// TorentMakerConsoleApplication.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "FileDialogEventsImpl.h"
#include "FFmpegIoCtx.h"
#include "DxHelpres\DxHelpers.h"
#include "ResourceFactory\DxResources.h"
#include "Texture\Yuv420pTexture.h"
#include "Texture\Bgra8CopyTexture.h"
#include "Texture\Bgra8RenderTarget.h"
#include "Helpers\FFmpegHelpers.h"
#include "Helpers\FileIterator.h"

#include <iostream>
#include <Windows.h>
#include <wrl.h>
#include <ShObjIdl.h>
#include <libhelpers\H.h>
#include <libhelpers\CoUniquePtr.h>
#include <libhelpers\ImageUtils.h>
#include <libhelpers\unique_ptr_extensions.h>
#include <libhelpers\ScopedValue.h>

extern "C" {
#include <libavformat\avformat.h>
}

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
		FileIterator fileIt(path);

		while (fileIt.Next()) {
			auto filePath = fileIt.GetCurrent();

			if (filePath.find(L".txt") != std::wstring::npos) {
				int stop = 234;
			}

			try {
				std::wcout << L"File: " << filePath << std::endl;

				// TODO test memleak on .zip or another unsupported files.

				int ffmpegRes = 0;
				FFmpegIoCtx ioCtx(filePath);

				auto fmtCtx = WrapUnique(avformat_alloc_context(), [](AVFormatContext *v) { avformat_close_input(&v); });
				fmtCtx->pb = ioCtx.GetAvioCtx();

				auto tmpNameUtf8 = H::Text::ConvertToUTF8(L"anykey" + ioCtx.GetExtension());

				ffmpegRes = avformat_open_input(GetAddressOf(fmtCtx), tmpNameUtf8.c_str(), nullptr, nullptr);
				FFmpegHelpers::ThrowIfFFmpegFailed(ffmpegRes);

				ffmpegRes = avformat_find_stream_info(fmtCtx.get(), nullptr);
				FFmpegHelpers::ThrowIfFFmpegFailed(ffmpegRes);

				av_format_inject_global_side_data(fmtCtx.get());

				AVCodec *dec = nullptr;
				auto decCtx = WrapUnique((AVCodecContext *)nullptr, [](AVCodecContext *v) { avcodec_close(v); });
				auto videoStreamIdx = av_find_best_stream(fmtCtx.get(), AVMediaType::AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
				FFmpegHelpers::ThrowIfFFmpegFailed(videoStreamIdx);

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
						FFmpegHelpers::ThrowIfFFmpegFailed(ffmpegRes);
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
				}
				else {
					duration = fmtCtx->duration;
					durationUnits = FFmpegHelpers::AVTimeBaseQ;
					streamUnits = stream->time_base;
				}

				int64_t timeStep = duration / (photoCount + 2); // +2 for begin, end
				int64_t endTime = duration - timeStep;

				// TODO add logic for handling files with 0 duration(images and even .txt(! yes ffmpeg can hadle this))
				//int photo = 0;

				for (int64_t time = timeStep; time < endTime; time += timeStep) {
					int64_t framePts = 0;
					int64_t curPts = 0;
					int64_t dstPts = av_rescale_q_rnd(time, durationUnits, streamUnits, AV_ROUND_UP);
					int droppedFrames = 0;

					if (!(seekFlags & AVSEEK_FLAG_BYTE)) {
						avcodec_flush_buffers(decCtx.get());
						ffmpegRes = av_seek_frame(fmtCtx.get(), seekStreamIdx, time, seekFlags);
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

					// TODO add logic when frame is empty

					DxDevice dxDev;
					DxResources dxRes;
					auto d3dDev = dxDev.GetD3DDevice();
					auto d3dCtx = dxDev.GetD3DContext();

					Yuv420pTexture<D3D11_USAGE_IMMUTABLE> yuvTex(
						d3dDev,
						DirectX::XMUINT2((uint32_t)frame->width, (uint32_t)frame->height),
						FFmpegHelpers::GetData<3>(frame.get()));

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

					auto pointSampler = dxRes.Samplers.GetPointSampler(d3dDev);
					auto linearSampler = dxRes.Samplers.GetLinearSampler(d3dDev);

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
			}
			catch (...) {
				std::cout << std::endl;
				std::cout << "Failed to process ";
				std::wcout << filePath;
				std::cout << " file." << std::endl;
			}
		} // while (fileIt.Next())
	} // while(true)

	return 0;
}