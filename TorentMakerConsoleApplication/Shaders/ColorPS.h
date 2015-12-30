#pragma once
#include "ShaderCBuffer.h"
#include "..\DxHelpres\DxHelpers.h"

class ColorPSCBuffer : public ShaderCBuffer {
public:
	ColorPSCBuffer(ID3D11Device *d3dDev);
	void Update(ID3D11DeviceContext *d3dCtx, const float color[4]);
};

class ColorPS {
public:
	ColorPS(ID3D11Device *d3dDev);

	void SetToContext(ID3D11DeviceContext *d3dCtx, const ColorPSCBuffer &cbuffer) const;

	static ColorPSCBuffer CreateCBuffer(ID3D11Device *d3dDev);

private:
	Microsoft::WRL::ComPtr<ID3D11PixelShader> shader;
};