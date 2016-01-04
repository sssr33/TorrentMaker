// TorentMakerConsoleApplication.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "FileDialogEventsImpl.h"
#include "FFmpegIoCtx.h"
#include "SequentialVideoFrameReader.h"
#include "DxHelpres\Dx.h"
#include "ResourceFactory\DxResources.h"
#include "Texture\Yuv420pTexture.h"
#include "Texture\Bgra8CopyTexture.h"
#include "Texture\Bgra8RenderTarget.h"
#include "Helpers\FFmpegHelpers.h"
#include "Helpers\FileIterator.h"
#include "Window\SimpleDxWindow.h"

#include <iostream>
#include <Windows.h>
#include <wrl.h>
#include <ShObjIdl.h>
#include <libhelpers\H.h>
#include <libhelpers\CoUniquePtr.h>
#include <libhelpers\ImageUtils.h>
#include <libhelpers\unique_ptr_extensions.h>
#include <libhelpers\ScopedValue.h>

#include <libhelpers\Thread\PPL\critical_section_guard_shared.h>
#include <libhelpers\Thread\PPL\critical_section_guard_unique.h>

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
		Dx dx;
		auto d3dDev = dx.dev.GetD3DDevice();
		//SimpleDxWindow window(dxDev);

		auto geometry = dx.res.Geometry.GetQuadStripIdx(d3dDev);

		auto vs = dx.res.Shaders.VS.GetQuadStripFltIndexVs(d3dDev);
		auto vsCBuffer = vs->CreateCBuffer(d3dDev);

		auto ps = dx.res.Shaders.PS.GetYuv420pToRgbaPS(d3dDev);
		auto colorPs = dx.res.Shaders.PS.GetColorPS(d3dDev);
		auto colorPsCBuffer = colorPs->CreateCBuffer(d3dDev);

		{
			auto ctx = dx.dev.GetContext();
			colorPsCBuffer.Update(ctx->D3D(), DirectX::Colors::Red);

			//auto testFn = [&]() {
			//	auto d2 = ctx->D2D();

			//	Sleep(10);

			//	HRESULT hr = S_OK;
			//	Microsoft::WRL::ComPtr<ID2D1Bitmap> bmp;
			//	auto bmpSize = D2D1::SizeU(1024, 1024);
			//	auto bmpProp = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

			//	d2->BeginDraw();
			//	Sleep(10);
			//	hr = d2->EndDraw();

			//	/*d2->SetTransform(D2D1::IdentityMatrix());

			//	hr = d2->CreateBitmap(bmpSize, bmpProp, bmp.GetAddressOf());
			//	*/

			//	if (FAILED(hr)) {
			//		int stop = 234;
			//	}
			//};

			//std::thread a([&]() { while (true) testFn(); });
			//std::thread b([&]() { while (true) testFn(); });

			//while (true)
			//{

			//}
		}

		auto pointSampler = dx.res.Samplers.GetPointSampler(d3dDev);
		auto linearSampler = dx.res.Samplers.GetLinearSampler(d3dDev);

		float angle = 0.0f;
		DirectX::XMMATRIX projection;

		DxWindow wnd(&dx);

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

				const auto frameSize = videoReader.GetFrameSize();
				Yuv420pTexture<D3D11_USAGE_DYNAMIC> yuvTex(
					d3dDev,
					frameSize);

				auto bgraTex = std::make_shared<Bgra8RenderTarget>(d3dDev,
					frameSize,
					0);

				/*Bgra8RenderTarget bgraTex(
					d3dDev,
					frameSize,
					0);*/
				Bgra8CopyTexture bgraTexCopy(
					d3dDev,
					frameSize);

				DirectX::XMMATRIX transform = DirectX::XMMatrixIdentity();

				transform = DirectX::XMMatrixMultiply(transform, DirectX::XMMatrixScaling(2, 2, 1));
				transform = DirectX::XMMatrixMultiply(transform, DirectX::XMMatrixTranslation(0, 0, 1));
				transform = DirectX::XMMatrixTranspose(transform);

				// TODO add logic for handling files with 0 duration(images and even .txt(! yes ffmpeg can hadle this))
				//int photo = 0;

				for (; videoReader.GetProgress() < endTime; videoReader.IncrementProgress(timeStep)) {
					{
						// TODO add logic when frame is empty
						auto frame = videoReader.GetFrame();
						auto ctx = dx.dev.GetContext();

						yuvTex.Update(ctx->D3D(), FFmpegHelpers::GetData<3>(frame));
						vsCBuffer.Update(ctx->D3D(), transform);

						RenderTargetState<1> rtState(ctx->D3D());
						bgraTex->Clear(ctx->D3D(), DirectX::Colors::White);
						bgraTex->SetRenderTarget(ctx->D3D());
						bgraTex->SetViewport(ctx->D3D());

						InputAssembler::Set(ctx->D3D(), *geometry, *vs, vsCBuffer);
						ps->SetToContext(ctx->D3D(), yuvTex, pointSampler);

						ctx->D3D()->Draw(geometry->GetVertexCount(), 0);

						bgraTex->GenerateMips(ctx->D3D());

						wnd.SetTexture(bgraTex);

						bgraTexCopy.Copy(ctx->D3D(), bgraTex.get());
						auto bgraTexCopyData = bgraTexCopy.Map(ctx->D3D());
						ImageUtils imgUtils;

						auto savePath = H::System::GetPackagePath() + L"screen0.png";
						auto encoder = imgUtils.CreateEncoder(savePath, GUID_ContainerFormatPng);
						auto encodeFrame = imgUtils.CreateFrameForEncode(encoder.Get());

						imgUtils.EncodeAllocPixels(encodeFrame.Get(), frameSize, GUID_WICPixelFormat32bppBGRA);
						imgUtils.EncodePixels(
							encodeFrame.Get(), frameSize.y,
							bgraTexCopyData.GetRowPitch(),
							bgraTexCopyData.GetRowPitch() * frameSize.y * 4,
							bgraTexCopyData.GetData());

						imgUtils.EncodeCommit(encodeFrame.Get());
						imgUtils.EncodeCommit(encoder.Get());
					}

					/*while (true)
					{

					}*/
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