#include "OMBlendState.h"

OMBlendState::OMBlendState(ID3D11DeviceContext *d3dCtx)
	:d3dCtx(d3dCtx)
{
	this->d3dCtx->OMGetBlendState(this->state.GetAddressOf(), this->blendFactor, &this->sampleMask);
}

OMBlendState::OMBlendState(OMBlendState &&other)
	: d3dCtx(std::move(other.d3dCtx)),
	state(std::move(other.state)),
	sampleMask(std::move(other.sampleMask))
{
	this->blendFactor[0] = other.blendFactor[0];
	this->blendFactor[1] = other.blendFactor[1];
	this->blendFactor[2] = other.blendFactor[2];
	this->blendFactor[3] = other.blendFactor[3];

	other.d3dCtx = nullptr;
}

OMBlendState::~OMBlendState() {
	if (this->d3dCtx) {
		this->d3dCtx->OMSetBlendState(this->state.Get(), this->blendFactor, this->sampleMask);
	}
}

OMBlendState &OMBlendState::operator=(OMBlendState &&other) {
	if (this != &other) {
		this->d3dCtx = std::move(other.d3dCtx);
		this->state = std::move(other.state);
		this->sampleMask = std::move(other.sampleMask);

		this->blendFactor[0] = other.blendFactor[0];
		this->blendFactor[1] = other.blendFactor[1];
		this->blendFactor[2] = other.blendFactor[2];
		this->blendFactor[3] = other.blendFactor[3];

		other.d3dCtx = nullptr;
	}

	return *this;
}