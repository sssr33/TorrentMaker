#include "DxWindow.h"

DxWindow::DxWindow(DxDevice &dxDev)
	:dxDev(dxDev), swapChainSize(0, 0)
{
	auto size = this->GetSize();
	this->ResizeSwapChain(size);
}

DxWindow::~DxWindow() {
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

void DxWindow::ProcessMsg(uint32_t msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
	case WM_SIZE: {
		DirectX::XMUINT2 size;

		size.x = LOWORD(lparam);
		size.y = HIWORD(lparam);

		if (this->ResizeSwapChain(size)) {
			this->CreateSizeDependentResources(this->swapChainSize);
		}
		break;
	}
	default:
		break;
	}
}

bool DxWindow::ResizeSwapChain(const DirectX::XMUINT2 &size) {
	const uint32_t BufferCount = 2;
	HRESULT hr = S_OK;
	bool sizeChanged = false;
	DirectX::XMUINT2 sizeTmp;
	auto d3dDev = this->dxDev.GetD3DDevice();

	sizeTmp.x = (std::max)(size.x, 1U);
	sizeTmp.y = (std::max)(size.y, 1U);

	if (this->swapChainSize.x != sizeTmp.x ||
		this->swapChainSize.y != sizeTmp.y)
	{
		sizeChanged = true;

		this->rtv = nullptr;
		this->swapChainSize = sizeTmp;

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