#include "SamplerFactory.h"

#include <libhelpers\H.h>

Microsoft::WRL::ComPtr<ID3D11SamplerState> SamplerFactory::GetPointSampler(ID3D11Device *d3dDev) {
	if (!this->pointSampler) {
		HRESULT hr = S_OK;
		WD3D11_SAMPLER_DESC desc(D3D11_FILTER_MIN_MAG_MIP_POINT);

		hr = d3dDev->CreateSamplerState(&desc, pointSampler.GetAddressOf());
		H::System::ThrowIfFailed(hr);
	}

	return this->pointSampler;
}

Microsoft::WRL::ComPtr<ID3D11SamplerState> SamplerFactory::GetLinearSampler(ID3D11Device *d3dDev) {
	if (!this->linearSampler) {
		HRESULT hr = S_OK;
		WD3D11_SAMPLER_DESC desc;

		hr = d3dDev->CreateSamplerState(&desc, linearSampler.GetAddressOf());
		H::System::ThrowIfFailed(hr);
	}

	return this->linearSampler;
}