#pragma once
#include "DxWindow.h"

class SimpleDxWindow : public DxWindow {
public:
	SimpleDxWindow(DxDevice &dxDev);
	virtual ~SimpleDxWindow();

protected:
	virtual void CreateSizeDependentResources(const DirectX::XMUINT2 &newSize) override;

private:
};