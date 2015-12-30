#include "QuadStripFltIndexVs.h"

QuadStripFltIndexVsCBuffer::QuadStripFltIndexVsCBuffer(ID3D11Device *d3dDev)
	: ShaderCBuffer(d3dDev, sizeof(DirectX::XMMATRIX)) {
}

void QuadStripFltIndexVsCBuffer::Update(ID3D11DeviceContext *d3dCtx, DirectX::CXMMATRIX data) {
	this->Update(d3dCtx, data);
}




QuadStripFltIndexVs::QuadStripFltIndexVs(ID3D11Device *d3dDev) {
	HRESULT hr = S_OK;
	auto data = H::System::LoadPackageFile(L"QuadStripFromFltIndexVs.cso");

	hr = d3dDev->CreateVertexShader(data.data(), data.size(), nullptr, this->vs.GetAddressOf());
	H::System::ThrowIfFailed(hr);

	D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	hr = d3dDev->CreateInputLayout(inputDesc, ARRAY_SIZE(inputDesc), data.data(), data.size(), this->inputLayout.GetAddressOf());
	H::System::ThrowIfFailed(hr);
}

const Microsoft::WRL::ComPtr<ID3D11InputLayout> &QuadStripFltIndexVs::GetInputLayout() const {
	return this->inputLayout;
}

void QuadStripFltIndexVs::SetToContext(ID3D11DeviceContext *d3dCtx, const QuadStripFltIndexVsCBuffer &cbuffer) const {
	auto &cbuffer0 = cbuffer.GetBuffer();

	d3dCtx->VSSetShader(this->vs.Get(), nullptr, 0);
	d3dCtx->VSSetConstantBuffers(0, 1, cbuffer0.GetAddressOf());
}

QuadStripFltIndexVsCBuffer QuadStripFltIndexVs::CreateCBuffer(ID3D11Device *d3dDev) {
	return QuadStripFltIndexVsCBuffer(d3dDev);
}