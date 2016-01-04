#pragma once
#include "..\DxHelpres\DxHelpers.h"
#include "..\Texture\Bgra8RenderTarget.h"

class Tex1PS {
public:
	Tex1PS(ID3D11Device *d3dDev);

	void SetToContext(
		ID3D11DeviceContext *d3dCtx,
		const Bgra8RenderTarget &tex,
		const Microsoft::WRL::ComPtr<ID3D11SamplerState> &sampler) const;

private:
	Microsoft::WRL::ComPtr<ID3D11PixelShader> shader;
};