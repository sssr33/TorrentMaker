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
#include "StepTimer.h"

#include <iostream>
#include <Windows.h>
#include <wrl.h>
#include <ShObjIdl.h>
#include <libhelpers\H.h>
#include <libhelpers\CoUniquePtr.h>
#include <libhelpers\ImageUtils.h>
#include <libhelpers\unique_ptr_extensions.h>
#include <libhelpers\ScopedValue.h>
#include <libhelpers\Containers\comptr_array.h>

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
				/*auto screenListProj3D = DirectX::XMMatrixOrthographicLH(screenList3DSize.x, screenList3DSize.y, 0.1f, 10.0f);*/
				/*auto screenListProj = H::Math::ProjectionD2D(
					(float)screenListSize.x, (float)screenListSize.y, 
					screenListProj3D,
					&screenListProjInv);*/


				auto view = DirectX::XMMatrixLookToLH(DirectX::XMVectorSet(0, 0, 0, 1.0f), DirectX::g_XMIdentityR2, DirectX::g_XMIdentityR1);
				auto screenListProj3D = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(90.0f), (float)screenListSize.x / (float)screenListSize.y, 0.1f, 10.0f);

				screenListProj3D = DirectX::XMMatrixMultiply(view, screenListProj3D);

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

				/*{
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
				}*/

				DX::StepTimer timer;

				float simpleTimer = 0;

				while (true)
				{
					DirectX::XMFLOAT2 penumbraPos;
					DirectX::XMFLOAT2 frameHalfSize;
					float penumbraSize = 0.05f;
					auto colorPs = dx.res.Shaders.PS.GetColorPS(d3dDev);
					auto colorCb = colorPs->CreateCBuffer(d3dDev);

					auto gradient2DPs = dx.res.Shaders.PS.GetGradient2DPS(d3dDev);
					auto gradient2DCb = gradient2DPs->CreateCBuffer(d3dDev);

					auto alphaBlend = dx.res.Blend.GetAlphaBlendState(d3dDev);

					auto ctx = dx.dev.GetContext();
					bgraTexFinal->Clear(ctx->D3D(), DirectX::Colors::WhiteSmoke);

					RenderTargetState<1> rtState(ctx->D3D());
					bgraTexFinal->SetRenderTarget(ctx->D3D());
					bgraTexFinal->SetViewport(ctx->D3D());

					framePosTmp.x = framePosTmp.y = 0.0f;
					frame3DSize.x = (float)frameSize.x / (float)frameSize.y;
					frame3DSize.y = 1.0f;

					frameHalfSize.x = frame3DSize.x * 0.5f;
					frameHalfSize.y = frame3DSize.y * 0.5f;

					/*auto frameTransform = DirectX::XMMatrixMultiply(
						DirectX::XMMatrixScaling(frame3DSize.x, frame3DSize.y, 1.0f),
						DirectX::XMMatrixTranslation(framePosTmp.x, framePosTmp.y, 1.0f));*/
					auto frameTransform = DirectX::XMMatrixMultiply(DirectX::XMMatrixRotationZ(simpleTimer), DirectX::XMMatrixTranslation(framePosTmp.x, framePosTmp.y, 1.0f));
					auto frameTransformFinal = DirectX::XMMatrixMultiplyTranspose(frameTransform, screenListProj3D);


					float sinVal = (float)std::abs(std::sin(simpleTimer));

					{
						// shadow
						DirectX::XMVECTOR lightPos = DirectX::XMVectorSet(0, 0, -1, 1);
						float lightRadius = 0.1f;// *sinVal;
						DirectX::XMVECTOR groundPlane = DirectX::XMPlaneFromPointNormal(
							DirectX::XMVectorSet(0, 0, 1.2f, 1),
							DirectX::XMVectorSet(0, 0, -1, 0));

						DirectX::XMVECTOR geometryVerts[] = {
							DirectX::XMVectorSet(-0.5f, -0.5f, 0.0f, 1.0f),
							DirectX::XMVectorSet(-0.5f, 0.5f, 0.0f, 1.0f),
							DirectX::XMVectorSet(0.5f, 0.5f, 0.0f, 1.0f),
							DirectX::XMVectorSet(0.5f, -0.5f, 0.0f, 1.0f),
						};
						uint16_t geometryTri[] = {
							0, 1, 2,
							2, 1, 3
						};

						size_t bufIdx = 0;
						float alphaBuf[128];
						DirectX::XMVECTOR posBuf[128];

						std::vector<uint32_t> innerIdx;

						innerIdx.reserve(4);

						DirectX::XMVECTOR trGeometryVerts[ARRAY_SIZE(geometryVerts)];

						for (size_t i = 0; i < ARRAY_SIZE(geometryVerts); i++) {
							trGeometryVerts[i] = DirectX::XMVector3Transform(geometryVerts[i], frameTransform);
						}

						for (size_t i = 0; i < ARRAY_SIZE(trGeometryVerts); i++) {
							size_t prevI = i == 0 ? (ARRAY_SIZE(trGeometryVerts) - 1) : i - 1 ;
							size_t nextI = (i + 1) % ARRAY_SIZE(trGeometryVerts);
							size_t nextI2 = (i + 2) % ARRAY_SIZE(trGeometryVerts);
							DirectX::XMVECTOR edgePrev = trGeometryVerts[prevI];
							DirectX::XMVECTOR edgeP1 = trGeometryVerts[i];
							DirectX::XMVECTOR edgeP2 = trGeometryVerts[nextI];
							DirectX::XMVECTOR edgeP22 = trGeometryVerts[nextI2];

							DirectX::XMVECTOR shadowYVec = DirectX::XMVectorSubtract(edgeP2, edgeP1);

							auto shadowP1YVec = shadowYVec;// DirectX::XMVectorAdd(DirectX::XMVectorSubtract(edgeP2, edgeP1), DirectX::XMVectorSubtract(edgeP1, edgePrev));
							auto shadowP2YVec = shadowYVec;// DirectX::XMVectorAdd(DirectX::XMVectorSubtract(edgeP2, edgeP1), DirectX::XMVectorSubtract(edgeP22, edgeP2));

							DirectX::XMVECTOR shadowZVecP1 = DirectX::XMVectorSubtract(edgeP1, lightPos);
							DirectX::XMVECTOR shadowZVecP2 = DirectX::XMVectorSubtract(edgeP2, lightPos);

							DirectX::XMVECTOR shadowXVecP1Inner = 
								DirectX::XMVector3Normalize(
									DirectX::XMVector3Cross(shadowZVecP1, shadowP1YVec));
							DirectX::XMVECTOR shadowXVecP2Inner =
								DirectX::XMVector3Normalize(
									DirectX::XMVector3Cross(shadowZVecP2, shadowP2YVec));

							shadowXVecP1Inner = DirectX::XMVectorScale(shadowXVecP1Inner, lightRadius);
							shadowXVecP2Inner = DirectX::XMVectorScale(shadowXVecP2Inner, lightRadius);

							DirectX::XMVECTOR shadowXVecP1Outer =
								DirectX::XMVector3Normalize(
									DirectX::XMVector3Cross(shadowZVecP1, shadowYVec));
							DirectX::XMVECTOR shadowXVecP2Outer =
								DirectX::XMVector3Normalize(
									DirectX::XMVector3Cross(shadowZVecP2, shadowYVec));

							shadowXVecP1Outer = DirectX::XMVectorScale(shadowXVecP1Outer, -lightRadius);
							shadowXVecP2Outer = DirectX::XMVectorScale(shadowXVecP2Outer, -lightRadius);

							/*auto shadowXVecP1Outer = DirectX::XMVectorNegate(shadowXVecP1Inner);
							auto shadowXVecP2Outer = DirectX::XMVectorNegate(shadowXVecP2Inner);*/

							auto lightP1InnerPoint = DirectX::XMVectorAdd(shadowXVecP1Inner, lightPos);
							auto lightP2InnerPoint = DirectX::XMVectorAdd(shadowXVecP2Inner, lightPos);

							auto lightP1OuterPoint = DirectX::XMVectorAdd(shadowXVecP1Outer, lightPos);
							auto lightP2OuterPoint = DirectX::XMVectorAdd(shadowXVecP2Outer, lightPos);

							auto shadowP1InnerPoint = DirectX::XMPlaneIntersectLine(groundPlane, lightP1InnerPoint, edgeP1);
							auto shadowP2InnerPoint = DirectX::XMPlaneIntersectLine(groundPlane, lightP2InnerPoint, edgeP2);

							auto shadowP1OuterPoint = DirectX::XMPlaneIntersectLine(groundPlane, lightP1OuterPoint, edgeP1);
							auto shadowP2OuterPoint = DirectX::XMPlaneIntersectLine(groundPlane, lightP2OuterPoint, edgeP2);

							//
							innerIdx.push_back(bufIdx);
							alphaBuf[bufIdx] = 1.0f;
							posBuf[bufIdx++] = shadowP1InnerPoint;

							alphaBuf[bufIdx] = 0.0f;
							posBuf[bufIdx++] = shadowP1OuterPoint;

							alphaBuf[bufIdx] = 0.0f;
							posBuf[bufIdx++] = shadowP2OuterPoint;


							alphaBuf[bufIdx] = 1.0f;
							posBuf[bufIdx++] = shadowP1InnerPoint;

							alphaBuf[bufIdx] = 0.0f;
							posBuf[bufIdx++] = shadowP2OuterPoint;

							alphaBuf[bufIdx] = 1.0f;
							posBuf[bufIdx++] = shadowP2InnerPoint;
							//

							int stop = 34;
						}

						alphaBuf[bufIdx] = 1.0f;
						posBuf[bufIdx++] = posBuf[innerIdx[0]];

						alphaBuf[bufIdx] = 1.0f;
						posBuf[bufIdx++] = posBuf[innerIdx[1]];

						alphaBuf[bufIdx] = 1.0f;
						posBuf[bufIdx++] = posBuf[innerIdx[2]];


						alphaBuf[bufIdx] = 1.0f;
						posBuf[bufIdx++] = posBuf[innerIdx[0]];

						alphaBuf[bufIdx] = 1.0f;
						posBuf[bufIdx++] = posBuf[innerIdx[2]];

						alphaBuf[bufIdx] = 1.0f;
						posBuf[bufIdx++] = posBuf[innerIdx[3]];

						OMBlendState blendState(ctx->D3D());
						alphaBlend->SetToContext(ctx->D3D());

						DynamicGeometry shadowGeom(d3dDev, 128);
						auto shadowGeomVS = dx.res.Shaders.VS.GetDynamicGeometryVS(d3dDev);
						auto shadowGeomVSCBuffer = shadowGeomVS->CreateCBuffer(d3dDev);

						shadowGeom.Update(ctx->D3D(), posBuf, alphaBuf, bufIdx);

						gradient2DCb.Update(ctx->D3D(), DirectX::Colors::Black);
						shadowGeomVSCBuffer.Update(ctx->D3D(), DirectX::XMMatrixTranspose(screenListProj3D));

						InputAssembler::Set(ctx->D3D(), shadowGeom, *shadowGeomVS, shadowGeomVSCBuffer);
						gradient2DPs->SetToContext(ctx->D3D(), gradient2DCb);

						ctx->D3D()->Draw(shadowGeom.GetVertexCount(), 0);
					}

					{
						// main geometry
						/*colorCb.Update(ctx->D3D(), DirectX::Colors::Blue);
						vsCBuffer.Update(ctx->D3D(), frameTransformFinal);

						InputAssembler::Set(ctx->D3D(), *geometry, *vs, vsCBuffer);
						colorPs->SetToContext(ctx->D3D(), colorCb);

						ctx->D3D()->Draw(geometry->GetVertexCount(), 0);*/
					}


					simpleTimer += 0.01f;
					/*timer.Tick([]() {});*/
				}

				while (true)
				{

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