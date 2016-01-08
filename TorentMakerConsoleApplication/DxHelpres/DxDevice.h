#pragma once
#include "DxDeviceCtx.h"
#include "DxDeviceMt.h"

#include <d3d11.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <dxgi.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <wrl.h>

#include <libhelpers\Thread\PPL\critical_section_guard.h>

class DxDevice : public DxDeviceMt {
public:
	DxDevice();
	~DxDevice();

	// cs-protected
	critical_section_guard<DxDeviceCtx>::Accessor GetContext();

private:
	// cs-protected
	critical_section_guard<DxDeviceCtx> ctx;

	D3D_FEATURE_LEVEL featureLevel;

	void CreateDeviceIndependentResources();
	void CreateDeviceDependentResources();
	void EnableD3DDeviceMultithreading();
	void CreateD2DDevice();
	Microsoft::WRL::ComPtr<ID2D1DeviceContext> CreateD2DDeviceContext();
};