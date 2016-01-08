#include "DxDeviceMt.h"

IDWriteFactory *DxDeviceMt::GetDwriteFactory() const {
	return this->dwriteFactory.Get();
}

ID2D1Factory1 *DxDeviceMt::GetD2DFactory() const {
	return this->d2dFactory.Get();
}

ID3D11Device *DxDeviceMt::GetD3DDevice() const {
	return this->d3dDev.Get();
}

ID2D1Device *DxDeviceMt::GetD2DDevice() const {
	return this->d2dDevice.Get();
}

D2DCtxMt *DxDeviceMt::GetD2DCtxMt() {
	return &this->d2dCtxMt;
}