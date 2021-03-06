#pragma once
#include "ShaderCBuffer.h"
#include "..\DxHelpres\DxHelpers.h"

class QuadStripFltIndexVsCBuffer : public ShaderCBuffer {
public:
	QuadStripFltIndexVsCBuffer(ID3D11Device *d3dDev);
	void Update(ID3D11DeviceContext *d3dCtx, DirectX::CXMMATRIX data);
};

class QuadStripFltIndexVs {
public:
	QuadStripFltIndexVs(ID3D11Device *d3dDev);

	const Microsoft::WRL::ComPtr<ID3D11InputLayout> &GetInputLayout() const;

	void SetToContext(ID3D11DeviceContext *d3dCtx, const QuadStripFltIndexVsCBuffer &cbuffer) const;

	static QuadStripFltIndexVsCBuffer CreateCBuffer(ID3D11Device *d3dDev);

private:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
};