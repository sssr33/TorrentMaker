#include "Yuv420pToRgbaPS.h"

#include <libhelpers\H.h>

Yuv420pToRgbaPS::Yuv420pToRgbaPS(ID3D11Device *d3dDev) {
	HRESULT hr = S_OK;
	auto data = H::System::LoadPackageFile(L"Yuv420pToRgbaPS.cso");

	hr = d3dDev->CreatePixelShader(data.data(), data.size(), nullptr, this->shader.GetAddressOf());
	H::System::ThrowIfFailed(hr);
}