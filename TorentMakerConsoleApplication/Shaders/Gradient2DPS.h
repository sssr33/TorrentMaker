#pragma once
#include "ShaderCBuffer.h"
#include "..\DxHelpres\DxHelpers.h"

class Gradient2DPSCBuffer : public ShaderCBuffer {
public:
	Gradient2DPSCBuffer(ID3D11Device *d3dDev);
	void Update(ID3D11DeviceContext *d3dCtx, const float color[4]);
};

class Gradient2DPS {
public:
	Gradient2DPS(ID3D11Device *d3dDev);

	void SetToContext(ID3D11DeviceContext *d3dCtx, const Gradient2DPSCBuffer &cbuffer) const;

	static Gradient2DPSCBuffer CreateCBuffer(ID3D11Device *d3dDev);

private:
	Microsoft::WRL::ComPtr<ID3D11PixelShader> shader;
};