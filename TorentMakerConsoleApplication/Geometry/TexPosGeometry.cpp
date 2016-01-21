#include "TexPosGeometry.h"

TexPosGeometry::TexPosGeometry(ID3D11Device *d3dDev, uint32_t maxVertexCount, uint32_t maxIndexCount) {
	HRESULT hr = S_OK;
	D3D11_BUFFER_DESC bufDesc;

	bufDesc.ByteWidth = maxVertexCount * sizeof(DirectX::XMFLOAT4);
	bufDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufDesc.MiscFlags = 0;
	bufDesc.StructureByteStride = 0;

	/*hr = d3dDev->CreateBuffer(&bufDesc, nullptr, this->posBuf.GetAddressOf());
	H::System::ThrowIfFailed(hr);



	bufDesc.ByteWidth = maxVertexCount * sizeof(DirectX::XMFLOAT2);

	hr = d3dDev->CreateBuffer(&bufDesc, nullptr, this->texBuf.GetAddressOf());
	H::System::ThrowIfFailed(hr);*/



	bufDesc.ByteWidth = maxIndexCount * sizeof(uint16_t);
	bufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	hr = d3dDev->CreateBuffer(&bufDesc, nullptr, this->idxBuf.GetAddressOf());
	H::System::ThrowIfFailed(hr);
}

void TexPosGeometry::SetToContext(ID3D11DeviceContext *d3dCtx, uint32_t startSlot) const {

}

uint32_t TexPosGeometry::GetIndexCount() {
	return 0;
}

D3D11_PRIMITIVE_TOPOLOGY TexPosGeometry::GetTopology() {
	return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}