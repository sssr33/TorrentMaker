#include "DxWindow.h"

DxWindow::DxWindow(Dx *dx)
	:dx(dx), swapChainSize(0, 0), work(true), texSize(1, 1)
{
	auto size = this->GetSize();
	this->ResizeSwapChain(size);
	this->CreateSizeDependentResources(this->swapChainSize);

	this->renderThread = std::thread([=]() {
		this->RenderMain();
	});
}

DxWindow::~DxWindow() {
	this->work = false;

	if (this->renderThread.joinable()) {
		this->renderThread.join();
	}
}

DirectX::XMUINT2 DxWindow::GetOutputSize() const {
	return this->swapChainSize;
}

void DxWindow::Clear(ID3D11DeviceContext *d3dCtx, const float color[4]) {
	d3dCtx->ClearRenderTargetView(this->rtv.Get(), color);
}

RenderTargetState<1> DxWindow::SetToContext(ID3D11DeviceContext *d3dCtx, ID3D11DepthStencilView *dsv) {
	RenderTargetState<1> state(d3dCtx);
	D3D11_VIEWPORT viewport;

	viewport.TopLeftX = viewport.TopLeftY = 0;
	viewport.Width = (float)this->swapChainSize.x;
	viewport.Height = (float)this->swapChainSize.y;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	d3dCtx->OMSetRenderTargets(1, this->rtv.GetAddressOf(), dsv);
	d3dCtx->RSSetViewports(1, &viewport);

	return state;
}

void DxWindow::Present() {
	HRESULT hr = S_OK;

	hr = this->swapChain->Present(1, 0);
	H::System::ThrowIfFailed(hr);
}

void DxWindow::SetTexture(const std::shared_ptr<Texture2DResource> &v) {
	thread::critical_section::scoped_lock lk(this->texCs);
	this->texture = v;
	this->texSize = this->texture->GetSize();
}

void DxWindow::CreateSizeDependentResources(const DirectX::XMUINT2 &newSize) {
	float ar = (float)newSize.x / (float)newSize.y;
	auto projectionTmp = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(90.0f), ar, 0.01f, 10.0f);

	DirectX::XMStoreFloat4x4(&this->projection, projectionTmp);
}

void DxWindow::ProcessMsg(uint32_t msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
	case WM_SIZE: {
		DirectX::XMUINT2 size;

		size.x = LOWORD(lparam);
		size.y = HIWORD(lparam);

		{
			thread::critical_section::scoped_lock lk(this->renderCs);
			if (this->ResizeSwapChain(size)) {
				this->CreateSizeDependentResources(this->swapChainSize);
			}
		}
		
		break;
	}
	default:
		break;
	}
}

void DxWindow::RenderMain() {
	auto d3dDev = this->dx->dev.GetD3DDevice();
	auto geometry = this->dx->res.Geometry.GetQuadStripIdx(d3dDev);

	auto vs = this->dx->res.Shaders.VS.GetQuadStripFltIndexVs(d3dDev);
	auto vsCBuffer = vs->CreateCBuffer(d3dDev);

	auto ps = this->dx->res.Shaders.PS.GetTex1PS(d3dDev);
	auto colorPs = this->dx->res.Shaders.PS.GetColorPS(d3dDev);
	auto colorPsCBuffer = colorPs->CreateCBuffer(d3dDev);

	{
		auto ctx = this->dx->dev.GetContext();
		colorPsCBuffer.Update(ctx->D3D(), DirectX::Colors::Red);
	}

	auto pointSampler = this->dx->res.Samplers.GetPointSampler(d3dDev);
	auto linearSampler = this->dx->res.Samplers.GetLinearSampler(d3dDev);

	while (this->work)
	{
		{
			thread::critical_section::scoped_lock lk(this->renderCs);
			auto ctx = this->dx->dev.GetContext();

			this->Clear(ctx->D3D(), DirectX::Colors::CornflowerBlue);
			auto state = this->SetToContext(ctx->D3D());

			auto projection = DirectX::XMLoadFloat4x4(&this->projection);

			DirectX::XMMATRIX world;

			{
				DirectX::XMFLOAT2 screenRect, texRect;

				screenRect.x = 2.0f * (float)this->swapChainSize.x / (float)this->swapChainSize.y;
				screenRect.y = 2.0f;

				thread::critical_section::scoped_lock lk(this->texCs);

				texRect.x = (float)this->texSize.x / (float)this->texSize.y;
				texRect.y = 1.0f;

				texRect = H::Math::InscribeRectAR(texRect, screenRect);

				world = DirectX::XMMatrixScaling(texRect.x, texRect.y, 1.0f);
			}

			world = DirectX::XMMatrixMultiply(world, DirectX::XMMatrixTranslation(0.0f, 0.0f, 1.0f));

			auto mvp = DirectX::XMMatrixMultiplyTranspose(world, projection);

			vsCBuffer.Update(ctx->D3D(), mvp);

			InputAssembler::Set(ctx->D3D(), *geometry, *vs, vsCBuffer);

			{
				thread::critical_section::scoped_lock lk(this->texCs);

				if (this->texture) {
					ps->SetToContext(ctx->D3D(), *this->texture, linearSampler);
				}
				else {
					colorPs->SetToContext(ctx->D3D(), colorPsCBuffer);
				}
			}

			ctx->D3D()->Draw(geometry->GetVertexCount(), 0);
		}
		
		// no lock here for perfromance
		// looks like it's thread-safe
		this->Present();
	}
}

bool DxWindow::ResizeSwapChain(const DirectX::XMUINT2 &size) {
	const uint32_t BufferCount = 2;
	HRESULT hr = S_OK;
	bool sizeChanged = false;
	DirectX::XMUINT2 sizeTmp;
	auto d3dDev = this->dx->dev.GetD3DDevice();

	sizeTmp.x = (std::max)(size.x, 1U);
	sizeTmp.y = (std::max)(size.y, 1U);

	if (this->swapChainSize.x != sizeTmp.x ||
		this->swapChainSize.y != sizeTmp.y)
	{
		sizeChanged = true;

		this->rtv = nullptr;
		this->swapChainSize = sizeTmp;

		{
			auto ctx = this->dx->dev.GetContext();
			ctx->D3D()->Flush();
		}

		if (!this->swapChain) {
			DXGI_SWAP_CHAIN_DESC swapChainDesc;
			Microsoft::WRL::ComPtr<IDXGIFactory> dxgiFactory;

			hr = CreateDXGIFactory(IID_PPV_ARGS(dxgiFactory.GetAddressOf()));
			H::System::ThrowIfFailed(hr);

			swapChainDesc.BufferCount = BufferCount;
			swapChainDesc.BufferDesc.Width = this->swapChainSize.x;
			swapChainDesc.BufferDesc.Height = this->swapChainSize.y;
			swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			swapChainDesc.BufferDesc.RefreshRate.Numerator = 1;
			swapChainDesc.BufferDesc.RefreshRate.Denominator = 60;
			swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_STRETCHED;
			swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.Flags = 0;
			swapChainDesc.OutputWindow = this->GetHwnd();
			swapChainDesc.SampleDesc.Count = 1;
			swapChainDesc.SampleDesc.Quality = 0;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
			swapChainDesc.Windowed = TRUE;

			hr = dxgiFactory->CreateSwapChain(d3dDev, &swapChainDesc, this->swapChain.ReleaseAndGetAddressOf());
			H::System::ThrowIfFailed(hr);
		}
		else {
			hr = this->swapChain->ResizeBuffers(
				BufferCount,
				this->swapChainSize.x,
				this->swapChainSize.y,
				DXGI_FORMAT_UNKNOWN,
				0);
			H::System::ThrowIfFailed(hr);
		}

		Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
		hr = this->swapChain->GetBuffer(0, IID_PPV_ARGS(tex.GetAddressOf()));
		H::System::ThrowIfFailed(hr);

		hr = d3dDev->CreateRenderTargetView(tex.Get(), nullptr, this->rtv.ReleaseAndGetAddressOf());
		H::System::ThrowIfFailed(hr);
	}

	return sizeChanged;
}