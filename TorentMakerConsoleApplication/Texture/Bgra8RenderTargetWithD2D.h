#pragma once
#include "Texture2D.h"

class Bgra8RenderTargetWithD2D : public Texture2DRenderTargetWithD2D {
public:
	Bgra8RenderTargetWithD2D(DxDeviceMt *dxDevMt, const DirectX::XMUINT2 &size);
};