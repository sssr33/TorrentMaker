#include "MatrixCBuffer.h"

MatrixCBuffer::MatrixCBuffer(ID3D11Device *d3dDev)
	: ShaderCBuffer(d3dDev, sizeof(DirectX::XMMATRIX)) {
}

void MatrixCBuffer::Update(ID3D11DeviceContext *d3dCtx, DirectX::CXMMATRIX data) {
	this->UpdateInternal(d3dCtx, data);
}