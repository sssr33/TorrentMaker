#include "SamplerFactory.h"

#include <libhelpers\H.h>

Microsoft::WRL::ComPtr<ID3D11SamplerState> SamplerFactory::GetPointSampler(ID3D11Device *d3dDev) {
	thread::critical_section::scoped_lock lk(this->cs);

	if (!this->pointSampler) {
		HRESULT hr = S_OK;
		WD3D11_SAMPLER_DESC desc(D3D11_FILTER_MIN_MAG_MIP_POINT);

		hr = d3dDev->CreateSamplerState(&desc, pointSampler.GetAddressOf());
		H::System::ThrowIfFailed(hr);
	}

	return this->pointSampler;
}

Microsoft::WRL::ComPtr<ID3D11SamplerState> SamplerFactory::GetLinearSampler(ID3D11Device *d3dDev) {
	thread::critical_section::scoped_lock lk(this->cs);

	if (!this->linearSampler) {
		HRESULT hr = S_OK;
		WD3D11_SAMPLER_DESC desc;

		hr = d3dDev->CreateSamplerState(&desc, linearSampler.GetAddressOf());
		H::System::ThrowIfFailed(hr);
	}

	return this->linearSampler;
}