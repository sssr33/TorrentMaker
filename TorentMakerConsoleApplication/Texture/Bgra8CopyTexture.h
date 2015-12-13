#pragma once
#include "Texture2D.h"
#include "..\DxHelpres\MappedResource.h"

class Bgra8RenderTarget;

class Bgra8CopyTexture : public Texture2D {
public:
	Bgra8CopyTexture(ID3D11Device *d3dDev, const DirectX::XMUINT2 &size);

	void Copy(ID3D11DeviceContext *ctx, const Bgra8RenderTarget *src);
	MappedResource Map(ID3D11DeviceContext *ctx);
};