#include "Bgra8CopyTexture.h"
#include "Bgra8RenderTarget.h"

#include <libhelpers\H.h>

Bgra8CopyTexture::Bgra8CopyTexture(ID3D11Device *d3dDev, const DirectX::XMUINT2 &size)
{
	HRESULT hr = S_OK;
	D3D11_TEXTURE2D_DESC texDesc;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;

	texDesc.Width = size.x;
	texDesc.Height = size.y;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_STAGING;
	texDesc.BindFlags = 0;
	texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	texDesc.MiscFlags = 0;

	hr = d3dDev->CreateTexture2D(&texDesc, nullptr, tex.GetAddressOf());
	H::System::ThrowIfFailed(hr);

	this->AddPlane(tex);
}

void Bgra8CopyTexture::Copy(ID3D11DeviceContext *ctx, const Bgra8RenderTarget *src) {
	auto dstTex = this->GetTexture();
	auto srcTex = src->GetTexture();

	ctx->CopySubresourceRegion(dstTex, 0, 0, 0, 0, srcTex, 0, nullptr);
}

MappedResource Bgra8CopyTexture::Map(ID3D11DeviceContext *ctx) {
	return MappedResource(ctx, this->GetTexture(), D3D11_MAP_READ);
}