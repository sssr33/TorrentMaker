// TorentMakerConsoleApplication.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "FileDialogEventsImpl.h"
#include "FFmpegIoCtx.h"
#include "SequentialVideoFrameReader.h"
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

				SequentialVideoFrameReader videoReader(filePath);
				uint32_t photoCount = 10;
				uint64_t timeStep = videoReader.GetProgressDelta() / (photoCount + 2); // +2 for begin, end
				uint64_t endTime = videoReader.GetProgressDelta() - timeStep;

				videoReader.IncrementProgress(timeStep);

				// TODO add logic for handling files with 0 duration(images and even .txt(! yes ffmpeg can hadle this))
				//int photo = 0;

				for (; videoReader.GetProgress() < endTime; videoReader.IncrementProgress(timeStep)) {
					// TODO add logic when frame is empty

					auto frame = videoReader.GetFrame();
					DxDevice dxDev;
					DxResources dxRes;
					auto d3dDev = dxDev.GetD3DDevice();
					auto d3dCtx = dxDev.GetD3DContext();

					Yuv420pTexture<D3D11_USAGE_IMMUTABLE> yuvTex(
						d3dDev,
						DirectX::XMUINT2((uint32_t)frame->width, (uint32_t)frame->height),
						FFmpegHelpers::GetData<3>(frame));

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

					auto yuv420pToRgbaPs = dxRes.Shaders.PS.GetYuv420pToRgbaPS(d3dDev);

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

					yuv420pToRgbaPs->SetToContext(d3dCtx, yuvTex, pointSampler);

					d3dCtx->Draw(4, 0);

					bgraTexCopy.Copy(d3dCtx, &bgraTex);
					auto bgraTexCopyData = bgraTexCopy.Map(d3dCtx);
					ImageUtils imgUtils;

					auto savePath = H::System::GetPackagePath() + L"screen0.png";
					auto encoder = imgUtils.CreateEncoder(savePath, GUID_ContainerFormatPng);
					auto encodeFrame = imgUtils.CreateFrameForEncode(encoder.Get());

					imgUtils.EncodeAllocPixels(encodeFrame.Get(), DirectX::XMUINT2(frame->width, frame->height), GUID_WICPixelFormat32bppBGRA);
					imgUtils.EncodePixels(encodeFrame.Get(), frame->height, bgraTexCopyData.GetRowPitch(), bgraTexCopyData.GetRowPitch() * frame->height * 4, bgraTexCopyData.GetData());

					imgUtils.EncodeCommit(encodeFrame.Get());
					imgUtils.EncodeCommit(encoder.Get());
				}

				auto videoInfo = videoReader.End();

				double durationSec =
					(double)(videoInfo.duration * videoInfo.durationUnits.num) /
					(double)videoInfo.durationUnits.den;

				std::cout << "Duration(s): " << durationSec << std::endl;
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