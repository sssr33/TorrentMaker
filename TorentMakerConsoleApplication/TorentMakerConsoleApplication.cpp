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

				Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvY, srvU, srvV;
				Microsoft::WRL::ComPtr<ID3D11Texture2D> texBGRA;
				Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvBGRA;
				Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtvBGRA;
				Microsoft::WRL::ComPtr<ID3D11Texture2D> screenTex;

				Microsoft::WRL::ComPtr<ID3D11Buffer> fltIdxBuf;

				Microsoft::WRL::ComPtr<ID3D11InputLayout> fltIdxInputLayout;
				Microsoft::WRL::ComPtr<ID3D11Buffer> fltIdxCBuffer;
				Microsoft::WRL::ComPtr<ID3D11VertexShader> fltIdxVs;

				Microsoft::WRL::ComPtr<ID3D11PixelShader> yuv420pToRgbaPs;

				Microsoft::WRL::ComPtr<ID3D11SamplerState> pointSampler;
				Microsoft::WRL::ComPtr<ID3D11SamplerState> linearSampler;

				// yuv420p tex resources
				{
					D3D11_SUBRESOURCE_DATA subResData;
					D3D11_TEXTURE2D_DESC yuv420pTexDesc;
					D3D11_SHADER_RESOURCE_VIEW_DESC yuv420pSrvDesc;

					// Y
					yuv420pTexDesc.Width = decCtx->width;
					yuv420pTexDesc.Height = decCtx->height;
					yuv420pTexDesc.MipLevels = 1;
					yuv420pTexDesc.ArraySize = 1;
					yuv420pTexDesc.Format = DXGI_FORMAT_R8_UNORM;
					yuv420pTexDesc.SampleDesc.Count = 1;
					yuv420pTexDesc.SampleDesc.Quality = 0;
					yuv420pTexDesc.Usage = D3D11_USAGE_IMMUTABLE;
					yuv420pTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
					yuv420pTexDesc.CPUAccessFlags = 0;
					yuv420pTexDesc.MiscFlags = 0;

					subResData.pSysMem = frame->data[0];
					subResData.SysMemPitch = frame->linesize[0];
					subResData.SysMemSlicePitch = 0;

					yuv420pSrvDesc.Format = yuv420pTexDesc.Format;
					yuv420pSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
					yuv420pSrvDesc.Texture2D.MipLevels = yuv420pTexDesc.MipLevels;
					yuv420pSrvDesc.Texture2D.MostDetailedMip = 0;

					{
						Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
						hr = d3dDev->CreateTexture2D(&yuv420pTexDesc, &subResData, tex.GetAddressOf());
						H::System::ThrowIfFailed(hr);

						hr = d3dDev->CreateShaderResourceView(tex.Get(), &yuv420pSrvDesc, srvY.GetAddressOf());
						H::System::ThrowIfFailed(hr);
					}

					//U
					yuv420pTexDesc.Width = decCtx->width / 2;
					yuv420pTexDesc.Height = decCtx->height / 2;

					subResData.pSysMem = frame->data[1];
					subResData.SysMemPitch = frame->linesize[1];

					{
						Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
						hr = d3dDev->CreateTexture2D(&yuv420pTexDesc, &subResData, tex.GetAddressOf());
						H::System::ThrowIfFailed(hr);

						hr = d3dDev->CreateShaderResourceView(tex.Get(), &yuv420pSrvDesc, srvU.GetAddressOf());
						H::System::ThrowIfFailed(hr);
					}

					//V
					subResData.pSysMem = frame->data[2];
					subResData.SysMemPitch = frame->linesize[2];

					{
						Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
						hr = d3dDev->CreateTexture2D(&yuv420pTexDesc, &subResData, tex.GetAddressOf());
						H::System::ThrowIfFailed(hr);

						hr = d3dDev->CreateShaderResourceView(tex.Get(), &yuv420pSrvDesc, srvV.GetAddressOf());
						H::System::ThrowIfFailed(hr);
					}
				}

				// bgra tex resources
				{
					D3D11_TEXTURE2D_DESC texDesc;
					D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
					D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;

					texDesc.Width = decCtx->width;
					texDesc.Height = decCtx->height;
					texDesc.MipLevels = 0;
					texDesc.ArraySize = 1;
					texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
					texDesc.SampleDesc.Count = 1;
					texDesc.SampleDesc.Quality = 0;
					texDesc.Usage = D3D11_USAGE_DEFAULT;
					texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
					texDesc.CPUAccessFlags = 0;
					texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

					srvDesc.Format = texDesc.Format;
					srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
					srvDesc.Texture2D.MipLevels = -1;
					srvDesc.Texture2D.MostDetailedMip = 0;

					rtvDesc.Format = texDesc.Format;
					rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
					rtvDesc.Texture2D.MipSlice = 0;

					hr = d3dDev->CreateTexture2D(&texDesc, nullptr, texBGRA.GetAddressOf());
					H::System::ThrowIfFailed(hr);

					hr = d3dDev->CreateShaderResourceView(texBGRA.Get(), &srvDesc, srvBGRA.GetAddressOf());
					H::System::ThrowIfFailed(hr);

					hr = d3dDev->CreateRenderTargetView(texBGRA.Get(), &rtvDesc, rtvBGRA.GetAddressOf());
					H::System::ThrowIfFailed(hr);
				}

				// screen tex
				{
					D3D11_TEXTURE2D_DESC texDesc;

					texDesc.Width = decCtx->width;
					texDesc.Height = decCtx->height;
					texDesc.MipLevels = 1;
					texDesc.ArraySize = 1;
					texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
					texDesc.SampleDesc.Count = 1;
					texDesc.SampleDesc.Quality = 0;
					texDesc.Usage = D3D11_USAGE_STAGING;
					texDesc.BindFlags = 0;
					texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
					texDesc.MiscFlags = 0;

					hr = d3dDev->CreateTexture2D(&texDesc, nullptr, screenTex.GetAddressOf());
					H::System::ThrowIfFailed(hr);
				}

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

				d3dCtx->ClearRenderTargetView(rtvBGRA.Get(), DirectX::Colors::White);
				d3dCtx->OMSetRenderTargets(1, rtvBGRA.GetAddressOf(), nullptr);

				D3D11_VIEWPORT viewport;

				viewport.Width = (float)decCtx->width;
				viewport.Height = (float)decCtx->height;
				viewport.TopLeftX = viewport.TopLeftY = 0.0f;
				viewport.MinDepth = 0.0f;
				viewport.MaxDepth = 1.0f;

				d3dCtx->RSSetViewports(1, &viewport);

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
				d3dCtx->PSSetShaderResources(0, 1, srvY.GetAddressOf());
				d3dCtx->PSSetShaderResources(1, 1, srvU.GetAddressOf());
				d3dCtx->PSSetShaderResources(2, 1, srvV.GetAddressOf());

				d3dCtx->Draw(4, 0);

				d3dCtx->CopySubresourceRegion(screenTex.Get(), 0, 0, 0, 0, texBGRA.Get(), 0, nullptr);

				D3D11_MAPPED_SUBRESOURCE mappedSubRes;
				hr = d3dCtx->Map(screenTex.Get(), 0, D3D11_MAP_READ, 0, &mappedSubRes);
				H::System::ThrowIfFailed(hr);

				ImageUtils imgUtils;

				auto savePath = H::System::GetPackagePath() + L"screen.png";
				auto encoder = imgUtils.CreateEncoder(savePath, GUID_ContainerFormatPng);
				auto encodeFrame = imgUtils.CreateFrameForEncode(encoder.Get());

				imgUtils.EncodeAllocPixels(encodeFrame.Get(), DirectX::XMUINT2(decCtx->width, decCtx->height), GUID_WICPixelFormat32bppBGRA);
				imgUtils.EncodePixels(encodeFrame.Get(), decCtx->height, mappedSubRes.RowPitch, mappedSubRes.RowPitch * decCtx->height * 4, mappedSubRes.pData);

				imgUtils.EncodeCommit(encodeFrame.Get());
				imgUtils.EncodeCommit(encoder.Get());

				d3dCtx->Unmap(screenTex.Get(), 0);
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