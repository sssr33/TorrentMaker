#pragma once
#include "DxDeviceCtx.h"

#include <d3d11.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <dxgi.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <wrl.h>

#include <libhelpers\Thread\PPL\critical_section_guard.h>

class DxDevice {
public:
	DxDevice();
	~DxDevice();

	// multithreaded
	IDWriteFactory *GetDwriteFactory() const;
	ID2D1Factory1 *GetD2DFactory() const;
	ID3D11Device *GetD3DDevice() const;
	ID2D1Device *GetD2DDevice() const;

	// cs-protected
	critical_section_guard<DxDeviceCtx>::Accessor GetContext();
	/*ID3D11DeviceContext *GetD3DContext() const;
	ID2D1DeviceContext *GetD2DCtx() const;*/

private:
	// multithreaded
	Microsoft::WRL::ComPtr<IDWriteFactory> dwriteFactory;
	Microsoft::WRL::ComPtr<ID2D1Factory1> d2dFactory;
	Microsoft::WRL::ComPtr<ID3D11Device> d3dDev;
	Microsoft::WRL::ComPtr<ID2D1Device> d2dDevice;

	// cs-protected
	critical_section_guard<DxDeviceCtx> ctx;
	/*Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3dCtx;
	Microsoft::WRL::ComPtr<ID2D1DeviceContext> d2dCtx;*/

	D3D_FEATURE_LEVEL featureLevel;

	void CreateDeviceIndependentResources();
	void CreateDeviceDependentResources();
	void EnableD3DDeviceMultithreading();
	void CreateD2DDevice();
	Microsoft::WRL::ComPtr<ID2D1DeviceContext> CreateD2DDeviceContext();
};