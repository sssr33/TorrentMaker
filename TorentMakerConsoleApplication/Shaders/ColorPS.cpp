#include "ColorPS.h"

ColorPSCBuffer::ColorPSCBuffer(ID3D11Device *d3dDev)
	: ShaderCBuffer(d3dDev, sizeof(DirectX::XMFLOAT4)) {
}

void ColorPSCBuffer::Update(ID3D11DeviceContext *d3dCtx, const float color[4]) {
	this->Update(d3dCtx, color);
}