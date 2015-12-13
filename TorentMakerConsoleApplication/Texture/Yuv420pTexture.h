#pragma once
#include "Texture2D.h"

#include <libhelpers\H.h>

template<D3D11_USAGE Usage>
class Yuv420pTexture : public Texture2DResource {
public:
	Yuv420pTexture(ID3D11Device *d3dDev, const DirectX::XMUINT2 &size) {
		this->CreateTexture(d3dDev, size, nullptr, 0);
	}

	Yuv420pTexture(
		ID3D11Device *d3dDev, 
		const DirectX::XMUINT2 &size, 
		const D3D11_SUBRESOURCE_DATA *data,
		size_t dataCount) 
	{
		this->CreateTexture(d3dDev, size, data, dataCount);
	}

	Yuv420pTexture(
		ID3D11Device *d3dDev,
		const DirectX::XMUINT2 &size,
		const std::array<D3D11_SUBRESOURCE_DATA, 3> &data)
	{
		this->CreateTexture(d3dDev, size, data.data(), data.size());
	}

	virtual ~Yuv420pTexture() {
	}

private:

	void CreateTexture(
		ID3D11Device *d3dDev,
		const DirectX::XMUINT2 &size,
		const D3D11_SUBRESOURCE_DATA *data,
		size_t dataCount)
	{
		static const DirectX::XMUINT2 SizeDiv[] = {
			DirectX::XMUINT2(1, 1),
			DirectX::XMUINT2(2, 2),
			DirectX::XMUINT2(2, 2)
		};

		if (data && dataCount != ARRAY_SIZE(SizeDiv)) {
			H::System::ThrowIfFailed(E_FAIL);
		}

		HRESULT hr = S_OK;
		D3D11_TEXTURE2D_DESC texDesc;
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;

		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R8_UNORM;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = Usage;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;

		srvDesc.Format = texDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;

		for (uint32_t i = 0; i < ARRAY_SIZE(SizeDiv); i++) {
			const D3D11_SUBRESOURCE_DATA *subResData = nullptr;
			Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;

			texDesc.Width = size.x / SizeDiv[i].x;
			texDesc.Height = size.y / SizeDiv[i].y;

			if (data) {
				subResData = &data[i];
			}

			hr = d3dDev->CreateTexture2D(&texDesc, subResData, tex.GetAddressOf());
			H::System::ThrowIfFailed(hr);

			hr = d3dDev->CreateShaderResourceView(tex.Get(), &srvDesc, srv.GetAddressOf());
			H::System::ThrowIfFailed(hr);

			this->AddPlane(tex, srv);
		}
	}
};