#include "BlendState.h"

BlendState::BlendState(
	const Microsoft::WRL::ComPtr<ID3D11BlendState> &state,
	float blendFactor[4],
	uint32_t sampleMask)
	: state(state), sampleMask(sampleMask)
{
	this->blendFactor[0] = blendFactor[0];
	this->blendFactor[1] = blendFactor[1];
	this->blendFactor[2] = blendFactor[2];
	this->blendFactor[3] = blendFactor[3];
}

BlendState::BlendState(
	const Microsoft::WRL::ComPtr<ID3D11BlendState> &state,
	const DirectX::XMVECTORF32 &blendFactor,
	uint32_t sampleMask)
	: state(state), sampleMask(sampleMask)
{
	this->blendFactor[0] = blendFactor[0];
	this->blendFactor[1] = blendFactor[1];
	this->blendFactor[2] = blendFactor[2];
	this->blendFactor[3] = blendFactor[3];
}

void BlendState::SetToContext(ID3D11DeviceContext *d3dCtx) const {
	d3dCtx->OMSetBlendState(this->state.Get(), this->blendFactor, this->sampleMask);
}