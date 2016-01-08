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
#include "Texture\Bgra8RenderTargetWithD2D.h"
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
		auto d2dCtxMt = dx.dev.GetD2DCtxMt();

		auto geometry = dx.res.Geometry.GetQuadStripIdx(d3dDev);

		auto vs = dx.res.Shaders.VS.GetQuadStripFltIndexVs(d3dDev);
		auto vsCBuffer = vs->CreateCBuffer(d3dDev);

		auto ps = dx.res.Shaders.PS.GetYuv420pToRgbaPS(d3dDev);
		auto texPs = dx.res.Shaders.PS.GetTex1PS(d3dDev);

		auto pointSampler = dx.res.Samplers.GetPointSampler(d3dDev);
		auto linearSampler = dx.res.Samplers.GetLinearSampler(d3dDev);

		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush, brush2;

		hr = d2dCtxMt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), brush.GetAddressOf());
		H::System::ThrowIfFailed(hr);

		hr = d2dCtxMt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LawnGreen), brush2.GetAddressOf());
		H::System::ThrowIfFailed(hr);

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
				DirectX::XMUINT2 photoCount(10, 10);
				uint32_t totalPhotoCount = photoCount.x * photoCount.y;
				uint64_t timeStep = videoReader.GetProgressDelta() / (totalPhotoCount + 2); // +2 for begin, end
				uint64_t endTime = videoReader.GetProgressDelta() - timeStep;
				DirectX::XMUINT2 screenListSize(1280, 720);
				const auto frameSize = videoReader.GetFrameSize();

				DirectX::XMFLOAT2 screenList3DSize, frame3DSize, frame3DPos, cell3DSize;

				screenList3DSize.x = 2.0f * (float)screenListSize.x / (float)screenListSize.y;
				screenList3DSize.y = 2.0f;

				frame3DSize.x = (float)frameSize.x / (float)frameSize.y;
				frame3DSize.y = 1.0f;

				cell3DSize.x = screenList3DSize.x / (float)photoCount.x;
				cell3DSize.y = screenList3DSize.y / (float)photoCount.y;

				frame3DPos.x = (-screenList3DSize.x + cell3DSize.x) * 0.5f;
				frame3DPos.y = (screenList3DSize.y - cell3DSize.y) * 0.5f;

				frame3DSize = H::Math::InscribeRectAR(frame3DSize, cell3DSize);

				frame3DSize.x *= 0.95f;
				frame3DSize.y *= 0.95f;

				D2D1::Matrix3x2F screenListProjInv;
				auto screenListProj3D = DirectX::XMMatrixOrthographicLH(screenList3DSize.x, screenList3DSize.y, 0.1f, 10.0f);
				auto screenListProj = H::Math::ProjectionD2D(
					(float)screenListSize.x, (float)screenListSize.y, 
					screenListProj3D,
					&screenListProjInv);

				videoReader.IncrementProgress(timeStep);

				Yuv420pTexture<D3D11_USAGE_DYNAMIC> yuvTex(
					d3dDev,
					frameSize);

				auto bgraTexFinal = std::make_shared<Bgra8RenderTargetWithD2D>(
					&dx.dev,
					screenListSize);

				auto bgraTex = std::make_shared<Bgra8RenderTarget>(&dx.dev,
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

				wnd.SetTexture(bgraTexFinal);

				auto framePosTmp = frame3DPos;
				auto frame3DHalfSize = frame3DSize;
				auto cell3DHalfSize = cell3DSize;

				frame3DHalfSize.x *= 0.5f;
				frame3DHalfSize.y *= 0.5f;

				cell3DHalfSize.x *= 0.5f;
				cell3DHalfSize.y *= 0.5f;

				{
					auto ctx = dx.dev.GetContext();
					bgraTexFinal->Clear(ctx->D3D(), DirectX::Colors::Black);
					bgraTexFinal->SetD2DRenderTarget(ctx->D2D());

					ctx->D2D()->BeginDraw();
					ctx->D2D()->SetTransform(screenListProj);

					for (uint32_t y = 0; y < photoCount.y; y++) {
						framePosTmp.x = frame3DPos.x;

						for (uint32_t x = 0; x < photoCount.x; x++) {
							auto cellLT = D2D1::Point2F(framePosTmp.x - cell3DHalfSize.x, framePosTmp.y - cell3DHalfSize.y);
							auto cellRT = D2D1::Point2F(framePosTmp.x + cell3DHalfSize.x, framePosTmp.y + cell3DHalfSize.y);
							auto frameLT = D2D1::Point2F(framePosTmp.x - frame3DHalfSize.x, framePosTmp.y - frame3DHalfSize.y);
							auto frameRT = D2D1::Point2F(framePosTmp.x + frame3DHalfSize.x, framePosTmp.y + frame3DHalfSize.y);

							ctx->D2D()->DrawRectangle(D2D1::RectF(cellLT.x, cellLT.y, cellRT.x, cellRT.y), brush.Get(), 0.005f);
							ctx->D2D()->DrawRectangle(D2D1::RectF(frameLT.x, frameLT.y, frameRT.x, frameRT.y), brush2.Get(), 0.005f);

							framePosTmp.x += cell3DSize.x;
						}

						framePosTmp.y -= cell3DSize.y;
					}

					ctx->D2D()->EndDraw();
				}

				framePosTmp = frame3DPos;

				// TODO add logic for handling files with 0 duration(images and even .txt(! yes ffmpeg can hadle this))
				//int photo = 0;

				//for (; videoReader.GetProgress() < endTime; videoReader.IncrementProgress(timeStep)) {
				for (uint32_t y = 0; y < photoCount.y; y++) {
					framePosTmp.x = frame3DPos.x;

					for (uint32_t x = 0; x < photoCount.x; x++, videoReader.IncrementProgress(timeStep)) {
						{
							// TODO add logic when frame is empty
							auto frame = videoReader.GetFrame();
							auto ctx = dx.dev.GetContext();

							yuvTex.Update(ctx->D3D(), FFmpegHelpers::GetData<3>(frame));
							vsCBuffer.Update(ctx->D3D(), transform);

							{
								RenderTargetState<1> rtState(ctx->D3D());
								bgraTex->Clear(ctx->D3D(), DirectX::Colors::White);
								bgraTex->SetRenderTarget(ctx->D3D());
								bgraTex->SetViewport(ctx->D3D());

								InputAssembler::Set(ctx->D3D(), *geometry, *vs, vsCBuffer);
								ps->SetToContext(ctx->D3D(), yuvTex, pointSampler);

								ctx->D3D()->Draw(geometry->GetVertexCount(), 0);
							}

							bgraTex->GenerateMips(ctx->D3D());

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

							{
								RenderTargetState<1> rtState(ctx->D3D());
								bgraTexFinal->SetRenderTarget(ctx->D3D());
								bgraTexFinal->SetViewport(ctx->D3D());

								auto frameTransform = DirectX::XMMatrixMultiply(
									DirectX::XMMatrixScaling(frame3DSize.x, frame3DSize.y, 1.0f), 
									DirectX::XMMatrixTranslation(framePosTmp.x, framePosTmp.y, 1.0f));
								frameTransform = DirectX::XMMatrixMultiplyTranspose(frameTransform, screenListProj3D);

								vsCBuffer.Update(ctx->D3D(), frameTransform);

								InputAssembler::Set(ctx->D3D(), *geometry, *vs, vsCBuffer);
								texPs->SetToContext(ctx->D3D(), *bgraTex, linearSampler);

								ctx->D3D()->Draw(geometry->GetVertexCount(), 0);
							}
						}

						framePosTmp.x += cell3DSize.x;
					} // x

					framePosTmp.y -= cell3DSize.y;
				} // y

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