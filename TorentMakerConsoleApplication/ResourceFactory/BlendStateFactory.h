#pragma once
#include "BlendState.h"
#include "..\DxHelpres\DxHelpers.h"

#include <memory>
#include <libhelpers\Thread\critical_section.h>

class BlendStateFactory {
public:
	std::shared_ptr<BlendState> GetDefaultState(ID3D11Device *d3dDev);
	std::shared_ptr<BlendState> GetAlphaBlendState(ID3D11Device *d3dDev);

private:
	thread::critical_section cs;

	std::shared_ptr<BlendState> defaultState;
	std::shared_ptr<BlendState> alphaBlendState;
};