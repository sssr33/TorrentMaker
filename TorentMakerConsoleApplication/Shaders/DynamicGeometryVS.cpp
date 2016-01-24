#include "DynamicGeometryVS.h"

DynamicGeometryVS::DynamicGeometryVS(ID3D11Device *d3dDev) {
	HRESULT hr = S_OK;
	auto data = H::System::LoadPackageFile(L"DynamicGeometryVS.cso");

	hr = d3dDev->CreateVertexShader(data.data(), data.size(), nullptr, this->shader.GetAddressOf());
	H::System::ThrowIfFailed(hr);

	D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	    { "TEXCOORD", 0, DXGI_FORMAT_R32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	hr = d3dDev->CreateInputLayout(inputDesc, ARRAY_SIZE(inputDesc), data.data(), data.size(), this->inputLayout.GetAddressOf());
	H::System::ThrowIfFailed(hr);
}

const Microsoft::WRL::ComPtr<ID3D11InputLayout> &DynamicGeometryVS::GetInputLayout() const {
	return this->inputLayout;
}

void DynamicGeometryVS::SetToContext(ID3D11DeviceContext *d3dCtx, const MatrixCBuffer &cbuffer) const {
	auto &cbuffer0 = cbuffer.GetBuffer();

	d3dCtx->VSSetShader(this->shader.Get(), nullptr, 0);
	d3dCtx->VSSetConstantBuffers(0, 1, cbuffer0.GetAddressOf());
}

MatrixCBuffer DynamicGeometryVS::CreateCBuffer(ID3D11Device *d3dDev) {
	return MatrixCBuffer(d3dDev);
}