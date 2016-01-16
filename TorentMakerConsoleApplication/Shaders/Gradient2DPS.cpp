#include "Gradient2DPS.h"

Gradient2DPSCBuffer::Gradient2DPSCBuffer(ID3D11Device *d3dDev)
	: ShaderCBuffer(d3dDev, sizeof(DirectX::XMFLOAT4)) {
}

void Gradient2DPSCBuffer::Update(ID3D11DeviceContext *d3dCtx, const float color[4]) {
	this->UpdateInternal(d3dCtx, color);
}




Gradient2DPS::Gradient2DPS(ID3D11Device *d3dDev) {
	HRESULT hr = S_OK;
	auto data = H::System::LoadPackageFile(L"Gradient2DPS.cso");

	hr = d3dDev->CreatePixelShader(data.data(), data.size(), nullptr, this->shader.GetAddressOf());
	H::System::ThrowIfFailed(hr);
}

void Gradient2DPS::SetToContext(ID3D11DeviceContext *d3dCtx, const Gradient2DPSCBuffer &cbuffer) const {
	auto &cbuffer0 = cbuffer.GetBuffer();

	d3dCtx->PSSetShader(this->shader.Get(), nullptr, 0);
	d3dCtx->PSSetConstantBuffers(0, 1, cbuffer0.GetAddressOf());
}

Gradient2DPSCBuffer Gradient2DPS::CreateCBuffer(ID3D11Device *d3dDev) {
	return Gradient2DPSCBuffer(d3dDev);
}