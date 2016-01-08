#pragma once

#include <d3d11.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <dxgi.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <wrl.h>

class DxDeviceCtx {
public:
	DxDeviceCtx();
	DxDeviceCtx(
		const Microsoft::WRL::ComPtr<ID3D11DeviceContext> &d3dCtx,
		const Microsoft::WRL::ComPtr<ID2D1DeviceContext> &d2dCtx);
	DxDeviceCtx(const DxDeviceCtx &other);
	DxDeviceCtx(DxDeviceCtx &&other);

	DxDeviceCtx &operator=(const DxDeviceCtx &other);
	DxDeviceCtx &operator=(DxDeviceCtx &&other);

	ID3D11DeviceContext *D3D() const;
	ID2D1DeviceContext *D2D() const;

private:
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3dCtx;
	Microsoft::WRL::ComPtr<ID2D1DeviceContext> d2dCtx;
};