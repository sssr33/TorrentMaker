#pragma once
#include "Window.h"
#include "..\DxHelpres\Dx.h"
#include "..\Texture\Bgra8RenderTarget.h"

#include <memory>

class DxWindow : public Window {
public:
	DxWindow(Dx *dx);
	virtual ~DxWindow();

	DirectX::XMUINT2 GetOutputSize() const;

	void Clear(ID3D11DeviceContext *d3dCtx, const float color[4]);
	RenderTargetState<1> SetToContext(ID3D11DeviceContext *d3dCtx, ID3D11DepthStencilView *dsv = nullptr);

	void Present();

	void SetTexture(const std::shared_ptr<Texture2DResource> &v);

protected:
	Dx *dx;

	virtual void CreateSizeDependentResources(const DirectX::XMUINT2 &newSize);

private:
	std::thread renderThread;
	thread::critical_section renderCs;
	bool work;

	DirectX::XMUINT2 swapChainSize;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv;

	float angle;
	DirectX::XMFLOAT4X4 projection;

	thread::critical_section texCs;
	DirectX::XMUINT2 texSize;
	std::shared_ptr<Texture2DResource> texture;

	virtual void ProcessMsg(uint32_t msg, WPARAM wparam, LPARAM lparam) override;

	void RenderMain();
	bool ResizeSwapChain(const DirectX::XMUINT2 &size);
};