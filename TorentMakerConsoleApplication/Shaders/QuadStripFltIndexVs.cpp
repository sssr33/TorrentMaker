#include "QuadStripFltIndexVs.h"

QuadStripFltIndexVsCBuffer::QuadStripFltIndexVsCBuffer(ID3D11Device *d3dDev) {
	HRESULT hr = S_OK;
	D3D11_BUFFER_DESC bufDesc;

	bufDesc.ByteWidth = sizeof(DirectX::XMMATRIX);
	bufDesc.Usage = D3D11_USAGE_DEFAULT;
	bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufDesc.CPUAccessFlags = 0;
	bufDesc.MiscFlags = 0;
	bufDesc.StructureByteStride = 0;

	hr = d3dDev->CreateBuffer(&bufDesc, nullptr, this->buffer.GetAddressOf());
	H::System::ThrowIfFailed(hr);
}

const Microsoft::WRL::ComPtr<ID3D11Buffer> &QuadStripFltIndexVsCBuffer::GetBuffer() const {
	return this->buffer;
}

void QuadStripFltIndexVsCBuffer::Update(ID3D11DeviceContext *d3dCtx, DirectX::CXMMATRIX data) {
	d3dCtx->UpdateSubresource(this->buffer.Get(), 0, nullptr, &data, 0, 0);
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