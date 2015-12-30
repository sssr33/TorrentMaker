#pragma once
#include "Window.h"
#include "..\DxHelpres\DxHelpers.h"

class DxWindow : public Window {
public:
	DxWindow(DxDevice &dxDev);
	virtual ~DxWindow();

	DirectX::XMUINT2 GetOutputSize() const;

	void Clear(ID3D11DeviceContext *d3dCtx, const float color[4]);
	RenderTargetState<1> SetToContext(ID3D11DeviceContext *d3dCtx, ID3D11DepthStencilView *dsv = nullptr);

	void Present();

protected:
	DxDevice &dxDev;

	virtual void CreateSizeDependentResources(const DirectX::XMUINT2 &newSize) = 0;

private:
	DirectX::XMUINT2 swapChainSize;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv;

	virtual void ProcessMsg(uint32_t msg, WPARAM wparam, LPARAM lparam) override;

	bool ResizeSwapChain(const DirectX::XMUINT2 &size);
};