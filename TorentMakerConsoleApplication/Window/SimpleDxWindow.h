#pragma once
#include "DxWindow.h"

#include <functional>

//class SimpleDxWindow : protected DxWindow {
//public:
//	class RenderScope {
//	public:
//		NO_COPY(RenderScope);
//
//		RenderScope(SimpleDxWindow *window, RenderTargetState<1> &&state);
//		RenderScope(RenderScope &&other);
//		~RenderScope();
//
//		RenderScope &operator=(RenderScope &&other);
//
//	private:
//		SimpleDxWindow *window;
//		RenderTargetState<1> state;
//	};
//
//	SimpleDxWindow(DxDevice &dxDev);
//	virtual ~SimpleDxWindow();
//
//	void SetOnSizeChanged(std::function<void(const DirectX::XMUINT2 &newSize)> v);
//
//	SimpleDxWindow::RenderScope Begin(ID3D11DeviceContext *d3dCtx, const float color[4]);
//
//protected:
//	virtual void CreateSizeDependentResources(const DirectX::XMUINT2 &newSize) override;
//
//private:
//
//	std::function<void(const DirectX::XMUINT2 &newSize)> onSizeChanged;
//};