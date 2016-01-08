#include "Tex1PS.h"

#include <libhelpers\H.h>

Tex1PS::Tex1PS(ID3D11Device *d3dDev) {
	HRESULT hr = S_OK;
	auto data = H::System::LoadPackageFile(L"Tex1PS.cso");

	hr = d3dDev->CreatePixelShader(data.data(), data.size(), nullptr, this->shader.GetAddressOf());
	H::System::ThrowIfFailed(hr);
}

void Tex1PS::SetToContext(
	ID3D11DeviceContext *d3dCtx,
	const Texture2DResource &tex,
	const Microsoft::WRL::ComPtr<ID3D11SamplerState> &sampler) const
{
	d3dCtx->PSSetShader(this->shader.Get(), nullptr, 0);
	d3dCtx->PSSetSamplers(0, 1, sampler.GetAddressOf());
	tex.SetToContextPS(d3dCtx, 0);
}