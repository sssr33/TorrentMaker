#pragma once
#include "..\DxHelpres\DxHelpers.h"

class BlendState {
public:
	BlendState(
		const Microsoft::WRL::ComPtr<ID3D11BlendState> &state, 
		float blendFactor[4], 
		uint32_t sampleMask);

	BlendState(
		const Microsoft::WRL::ComPtr<ID3D11BlendState> &state,
		const DirectX::XMVECTORF32 &blendFactor,
		uint32_t sampleMask);

	void SetToContext(ID3D11DeviceContext *d3dCtx) const;

private:
	Microsoft::WRL::ComPtr<ID3D11BlendState> state;
	float blendFactor[4];
	uint32_t sampleMask;
};