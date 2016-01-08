#pragma once
#include "Texture2D.h"

class Bgra8RenderTarget : public Texture2DRenderTarget {
public:
	Bgra8RenderTarget(DxDeviceMt *dxDevMt, const DirectX::XMUINT2 &size, uint32_t mips = 1);

	void GenerateMips(ID3D11DeviceContext *d3dCtx);
};