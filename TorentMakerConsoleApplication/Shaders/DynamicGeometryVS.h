#pragma once
#include "MatrixCBuffer.h"
#include "..\DxHelpres\DxHelpers.h"

class DynamicGeometryVS {
public:
	DynamicGeometryVS(ID3D11Device *d3dDev);

	const Microsoft::WRL::ComPtr<ID3D11InputLayout> &GetInputLayout() const;

	void SetToContext(ID3D11DeviceContext *d3dCtx, const MatrixCBuffer &cbuffer) const;

	static MatrixCBuffer CreateCBuffer(ID3D11Device *d3dDev);

private:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
};