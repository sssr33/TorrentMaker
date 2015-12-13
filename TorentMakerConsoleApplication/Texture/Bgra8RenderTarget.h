#pragma once
#include "Texture2D.h"

class Bgra8RenderTarget : public Texture2DRenderTarget {
public:
	Bgra8RenderTarget(ID3D11Device *d3dDev, const DirectX::XMUINT2 &size, uint32_t mips = 1);
};