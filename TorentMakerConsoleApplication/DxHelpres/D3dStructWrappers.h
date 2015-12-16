#pragma once

#include <d3d11.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <wrl.h>

struct WD3D11_SAMPLER_DESC : public D3D11_SAMPLER_DESC {
	WD3D11_SAMPLER_DESC();
	WD3D11_SAMPLER_DESC(D3D11_FILTER filter);
};