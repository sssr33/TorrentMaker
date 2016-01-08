#pragma once
#include "..\DxHelpres\DxHelpers.h"
#include "..\Texture\Yuv420pTexture.h"

class Yuv420pToRgbaPS {
public:
	Yuv420pToRgbaPS(ID3D11Device *d3dDev);

	template<D3D11_USAGE usage>
	void SetToContext(
		ID3D11DeviceContext *d3dCtx,
		const Yuv420pTexture<usage> &tex,
		const Microsoft::WRL::ComPtr<ID3D11SamplerState> &sampler) const
	{
		d3dCtx->PSSetShader(this->shader.Get(), nullptr, 0);
		d3dCtx->PSSetSamplers(0, 1, sampler.GetAddressOf());
		tex.SetToContextPS(d3dCtx, 0);
	}

private:
	Microsoft::WRL::ComPtr<ID3D11PixelShader> shader;
};