#pragma once
#include "..\DxHelpres\DxHelpers.h"

#include <libhelpers\Thread\critical_section.h>

class SamplerFactory {
public:
	Microsoft::WRL::ComPtr<ID3D11SamplerState> GetPointSampler(ID3D11Device *d3dDev);
	Microsoft::WRL::ComPtr<ID3D11SamplerState> GetLinearSampler(ID3D11Device *d3dDev);
private:
	thread::critical_section cs;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> pointSampler;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> linearSampler;
};