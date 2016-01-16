#include "BlendStateFactory.h"

#include <libhelpers\H.h>

std::shared_ptr<BlendState> BlendStateFactory::GetDefaultState(ID3D11Device *d3dDev) {
	thread::critical_section::scoped_lock lk(this->cs);

	if (!this->defaultState) {
		HRESULT hr = S_OK;
		D3D11_BLEND_DESC blendDesc;
		D3D11_RENDER_TARGET_BLEND_DESC rtBlend;
		Microsoft::WRL::ComPtr<ID3D11BlendState> state;

		rtBlend.BlendEnable = FALSE;
		rtBlend.SrcBlend = D3D11_BLEND_ONE;
		rtBlend.DestBlend = D3D11_BLEND_ZERO;
		rtBlend.BlendOp = D3D11_BLEND_OP_ADD;
		rtBlend.SrcBlendAlpha = D3D11_BLEND_ONE;
		rtBlend.DestBlendAlpha = D3D11_BLEND_ZERO;
		rtBlend.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		rtBlend.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = FALSE;

		for (auto &i : blendDesc.RenderTarget) {
			i = rtBlend;
		}

		hr = d3dDev->CreateBlendState(&blendDesc, state.GetAddressOf());
		H::System::ThrowIfFailed(hr);

		this->defaultState = std::make_shared<BlendState>(state, DirectX::g_XMOne, UINT_MAX);
	}

	return this->defaultState;
}

std::shared_ptr<BlendState> BlendStateFactory::GetAlphaBlendState(ID3D11Device *d3dDev) {
	thread::critical_section::scoped_lock lk(this->cs);

	if (!this->alphaBlendState) {
		HRESULT hr = S_OK;
		D3D11_BLEND_DESC blendDesc;
		D3D11_RENDER_TARGET_BLEND_DESC rtBlend;
		Microsoft::WRL::ComPtr<ID3D11BlendState> state;

		rtBlend.BlendEnable = TRUE;
		rtBlend.SrcBlend = D3D11_BLEND_SRC_ALPHA;
		rtBlend.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		rtBlend.BlendOp = D3D11_BLEND_OP_ADD;
		rtBlend.SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
		rtBlend.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		rtBlend.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		rtBlend.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = FALSE;

		for (auto &i : blendDesc.RenderTarget) {
			i = rtBlend;
		}

		hr = d3dDev->CreateBlendState(&blendDesc, state.GetAddressOf());
		H::System::ThrowIfFailed(hr);

		this->alphaBlendState = std::make_shared<BlendState>(state, DirectX::g_XMOne, UINT_MAX);
	}

	return this->alphaBlendState;
}