#pragma once
#include "..\DxHelpres\DxHelpers.h"

class DynamicGeometry {
public:
	DynamicGeometry(ID3D11Device *d3dDev, uint32_t vertexCapacity);

	uint32_t GetVertexCount() const;

	void Update(ID3D11DeviceContext *d3dCtx, const DirectX::XMVECTOR *pos, const float *alpha, uint32_t count);
	void SetToContext(ID3D11DeviceContext *d3dCtx, uint32_t startSlot = 0) const;

private:
	uint32_t vertexCapacity;
	uint32_t vertexCount;
	Microsoft::WRL::ComPtr<ID3D11Buffer> posBuf;
	Microsoft::WRL::ComPtr<ID3D11Buffer> alphaBuf;
};