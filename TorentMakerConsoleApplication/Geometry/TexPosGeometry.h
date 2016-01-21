#pragma once
#include "..\DxHelpres\DxHelpers.h"

#include <libhelpers\Containers\comptr_array.h>

class TexPosGeometry {
public:
	TexPosGeometry(ID3D11Device *d3dDev, uint32_t maxVertexCount, uint32_t maxIndexCount);

	void SetToContext(ID3D11DeviceContext *d3dCtx, uint32_t startSlot = 0) const;

	uint32_t GetIndexCount();

	static D3D11_PRIMITIVE_TOPOLOGY GetTopology();

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> idxBuf;
	comptr_array<ID3D11Buffer, 2> vertexBuf;

};