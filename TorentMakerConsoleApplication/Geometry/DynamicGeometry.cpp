#include "DynamicGeometry.h"

#include <libhelpers\H.h>

DynamicGeometry::DynamicGeometry(ID3D11Device *d3dDev, uint32_t vertexCapacity)
	: vertexCapacity(vertexCapacity), vertexCount(0)
{
	HRESULT hr = S_OK;
	D3D11_BUFFER_DESC bufDesc;

	bufDesc.ByteWidth = this->vertexCapacity * sizeof(DirectX::XMVECTOR);
	bufDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufDesc.MiscFlags = 0;
	bufDesc.StructureByteStride = 0;

	hr = d3dDev->CreateBuffer(&bufDesc, nullptr, this->posBuf.GetAddressOf());
	H::System::ThrowIfFailed(hr);

	bufDesc.ByteWidth = this->vertexCapacity * sizeof(float);

	hr = d3dDev->CreateBuffer(&bufDesc, nullptr, this->alphaBuf.GetAddressOf());
	H::System::ThrowIfFailed(hr);
}

uint32_t DynamicGeometry::GetVertexCount() const {
	return this->vertexCount;
}

void DynamicGeometry::Update(ID3D11DeviceContext *d3dCtx, const DirectX::XMVECTOR *pos, const float *alpha, uint32_t count) {
	H::System::Assert(count <= this->vertexCapacity);

	MappedResource mappedPos(d3dCtx, this->posBuf.Get(), D3D11_MAP_WRITE_DISCARD);
	MappedResource mappedAlpha(d3dCtx, this->alphaBuf.Get(), D3D11_MAP_WRITE_DISCARD);

	std::memcpy(mappedPos.GetData(), pos, count * sizeof(DirectX::XMVECTOR));
	std::memcpy(mappedAlpha.GetData(), alpha, count * sizeof(float));

	this->vertexCount = count;
}

void DynamicGeometry::SetToContext(ID3D11DeviceContext *d3dCtx, uint32_t startSlot) const {
	uint32_t stride = (uint32_t)sizeof(DirectX::XMVECTOR);
	uint32_t stride2 = (uint32_t)sizeof(float);
	uint32_t offset = 0;

	d3dCtx->IASetVertexBuffers(startSlot, 1, this->posBuf.GetAddressOf(), &stride, &offset);
	d3dCtx->IASetVertexBuffers(startSlot + 1, 1, this->alphaBuf.GetAddressOf(), &stride2, &offset);
}