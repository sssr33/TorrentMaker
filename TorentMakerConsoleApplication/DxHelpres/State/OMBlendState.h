#pragma once
#include "..\DxDevice.h"

#include <libhelpers\H.h>
#include <libhelpers\Containers\comptr_array.h>

class OMBlendState {
public:
	NO_COPY(OMBlendState);

	OMBlendState(ID3D11DeviceContext *d3dCtx);
	OMBlendState(OMBlendState &&other);
	~OMBlendState();

	OMBlendState &operator=(OMBlendState &&other);

private:
	ID3D11DeviceContext *d3dCtx;

	Microsoft::WRL::ComPtr<ID3D11BlendState> state;
	float blendFactor[4];
	uint32_t sampleMask;
};