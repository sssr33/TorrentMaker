#include "ShaderCBuffer.h"

ShaderCBuffer::ShaderCBuffer(ID3D11Device *d3dDev, uint32_t size) {
	HRESULT hr = S_OK;
	D3D11_BUFFER_DESC bufDesc;

	bufDesc.ByteWidth = size;
	bufDesc.Usage = D3D11_USAGE_DEFAULT;
	bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufDesc.CPUAccessFlags = 0;
	bufDesc.MiscFlags = 0;
	bufDesc.StructureByteStride = 0;

	hr = d3dDev->CreateBuffer(&bufDesc, nullptr, this->buffer.GetAddressOf());
	H::System::ThrowIfFailed(hr);
}

const Microsoft::WRL::ComPtr<ID3D11Buffer> &ShaderCBuffer::GetBuffer() const {
	return this->buffer;
}