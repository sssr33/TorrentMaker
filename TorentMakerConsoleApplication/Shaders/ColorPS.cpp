#include "ColorPS.h"

ColorPSCBuffer::ColorPSCBuffer(ID3D11Device *d3dDev)
	: ShaderCBuffer(d3dDev, sizeof(DirectX::XMFLOAT4)) {
}

void ColorPSCBuffer::Update(ID3D11DeviceContext *d3dCtx, const float color[4]) {
	this->UpdateInternal(d3dCtx, color);
}




ColorPS::ColorPS(ID3D11Device *d3dDev) {
	HRESULT hr = S_OK;
	auto data = H::System::LoadPackageFile(L"ColorPS.cso");

	hr = d3dDev->CreatePixelShader(data.data(), data.size(), nullptr, this->shader.GetAddressOf());
	H::System::ThrowIfFailed(hr);
}

void ColorPS::SetToContext(ID3D11DeviceContext *d3dCtx, const ColorPSCBuffer &cbuffer) const {
	auto &cbuffer0 = cbuffer.GetBuffer();

	d3dCtx->PSSetShader(this->shader.Get(), nullptr, 0);
	d3dCtx->PSSetConstantBuffers(0, 1, cbuffer0.GetAddressOf());
}

ColorPSCBuffer ColorPS::CreateCBuffer(ID3D11Device *d3dDev) {
	return ColorPSCBuffer(d3dDev);
}