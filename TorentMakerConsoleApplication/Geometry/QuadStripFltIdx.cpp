#include "QuadStripFltIdx.h"

QuadStripFltIdx::QuadStripFltIdx(ID3D11Device *d3dDev) {
	HRESULT hr = S_OK;
	float idx[] = { 0, 1, 2, 3 };
	D3D11_BUFFER_DESC bufDesc;
	D3D11_SUBRESOURCE_DATA subResData;

	bufDesc.ByteWidth = sizeof(idx);
	bufDesc.Usage = D3D11_USAGE_IMMUTABLE;
	bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufDesc.CPUAccessFlags = 0;
	bufDesc.MiscFlags = 0;
	bufDesc.StructureByteStride = 0;

	subResData.pSysMem = idx;
	subResData.SysMemPitch = 0;
	subResData.SysMemSlicePitch = 0;

	hr = d3dDev->CreateBuffer(&bufDesc, &subResData, fltIdxBuf.GetAddressOf());
	H::System::ThrowIfFailed(hr);
}

void QuadStripFltIdx::SetToContext(ID3D11DeviceContext *d3dCtx, uint32_t startSlot) const {
	uint32_t stride = (uint32_t)sizeof(float);
	uint32_t offset = 0;

	d3dCtx->IASetVertexBuffers(startSlot, 1, fltIdxBuf.GetAddressOf(), &stride, &offset);
}

D3D11_PRIMITIVE_TOPOLOGY QuadStripFltIdx::GetTopology() {
	return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
}

uint32_t QuadStripFltIdx::GetVertexCount() {
	return 4;
}