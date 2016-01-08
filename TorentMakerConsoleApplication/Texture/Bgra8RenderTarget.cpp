#include "Bgra8RenderTarget.h"

#include <libhelpers\H.h>

Bgra8RenderTarget::Bgra8RenderTarget(DxDeviceMt *dxDevMt, const DirectX::XMUINT2 &size, uint32_t mips)
{
	HRESULT hr = S_OK;
	auto d3dDev = dxDevMt->GetD3DDevice();
	auto d2dCtxMt = dxDevMt->GetD2DCtxMt();
	D3D11_TEXTURE2D_DESC texDesc;
	/*D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;*/
	Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv;

	texDesc.Width = size.x;
	texDesc.Height = size.y;
	texDesc.MipLevels = mips;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = mips != 1 ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

	/*srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = -1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	rtvDesc.Format = texDesc.Format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;*/

	hr = d3dDev->CreateTexture2D(&texDesc, nullptr, tex.GetAddressOf());
	H::System::ThrowIfFailed(hr);

	hr = d3dDev->CreateShaderResourceView(tex.Get(), nullptr, srv.GetAddressOf());
	H::System::ThrowIfFailed(hr);

	hr = d3dDev->CreateRenderTargetView(tex.Get(), nullptr, rtv.GetAddressOf());
	H::System::ThrowIfFailed(hr);

	this->AddPlane(tex, srv, rtv);
}

void Bgra8RenderTarget::GenerateMips(ID3D11DeviceContext *d3dCtx) {
	auto planeCount = this->GetPlaneCount();

	for (size_t i = 0; i < planeCount; i++) {
		d3dCtx->GenerateMips(this->GetShaderResourceView(i));
	}
}