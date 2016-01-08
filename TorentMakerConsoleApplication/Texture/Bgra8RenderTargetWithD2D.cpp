#include "Bgra8RenderTargetWithD2D.h"

Bgra8RenderTargetWithD2D::Bgra8RenderTargetWithD2D(DxDeviceMt *dxDevMt, const DirectX::XMUINT2 &size) {
	HRESULT hr = S_OK;
	auto d3dDev = dxDevMt->GetD3DDevice();
	auto d2dCtxMt = dxDevMt->GetD2DCtxMt();
	D3D11_TEXTURE2D_DESC texDesc;
	D2D1_BITMAP_PROPERTIES1 bitmapProps;
	D2D1_BITMAP_BRUSH_PROPERTIES1 brushProps;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv;
	Microsoft::WRL::ComPtr<IDXGISurface> dxgiSurface;
	Microsoft::WRL::ComPtr<ID2D1Bitmap1> bitmap;
	Microsoft::WRL::ComPtr<ID2D1BitmapBrush1> bitmapBrush;

	texDesc.Width = size.x;
	texDesc.Height = size.y;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	bitmapProps.pixelFormat.format = texDesc.Format;
	bitmapProps.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	bitmapProps.dpiX = bitmapProps.dpiY = 96.0f;
	bitmapProps.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;
	bitmapProps.colorContext = nullptr;

	brushProps.extendModeX = D2D1_EXTEND_MODE_CLAMP;
	brushProps.extendModeY = D2D1_EXTEND_MODE_CLAMP;
	brushProps.interpolationMode = D2D1_INTERPOLATION_MODE_LINEAR;

	hr = d3dDev->CreateTexture2D(&texDesc, nullptr, tex.GetAddressOf());
	H::System::ThrowIfFailed(hr);

	hr = d3dDev->CreateShaderResourceView(tex.Get(), nullptr, srv.GetAddressOf());
	H::System::ThrowIfFailed(hr);

	hr = d3dDev->CreateRenderTargetView(tex.Get(), nullptr, rtv.GetAddressOf());
	H::System::ThrowIfFailed(hr);

	hr = tex.As(&dxgiSurface);
	H::System::ThrowIfFailed(hr);

	hr = d2dCtxMt->CreateBitmapFromDxgiSurface(dxgiSurface.Get(), &bitmapProps, bitmap.GetAddressOf());
	H::System::ThrowIfFailed(hr);

	hr = d2dCtxMt->CreateBitmapBrush(bitmap.Get(), brushProps, bitmapBrush.GetAddressOf());
	H::System::ThrowIfFailed(hr);

	this->AddPlane(tex, srv, rtv, bitmap, bitmapBrush);
}