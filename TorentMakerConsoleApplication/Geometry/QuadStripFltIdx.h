#pragma once
#include "..\DxHelpres\DxHelpers.h"

class QuadStripFltIdx {
public:
	QuadStripFltIdx(ID3D11Device *d3dDev);

	void SetToContext(ID3D11DeviceContext *d3dCtx, uint32_t startSlot = 0) const;

	static D3D11_PRIMITIVE_TOPOLOGY GetTopology();
	static uint32_t GetVertexCount();

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> fltIdxBuf;
};