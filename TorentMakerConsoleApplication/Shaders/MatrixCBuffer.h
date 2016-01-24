#pragma once
#include "ShaderCBuffer.h"

class MatrixCBuffer : public ShaderCBuffer {
public:
	MatrixCBuffer(ID3D11Device *d3dDev);
	void Update(ID3D11DeviceContext *d3dCtx, DirectX::CXMMATRIX data);
};