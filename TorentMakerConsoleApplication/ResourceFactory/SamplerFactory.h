#pragma once
#include "..\DxHelpres\DxHelpers.h"

class SamplerFactory {
public:
	Microsoft::WRL::ComPtr<ID3D11SamplerState> GetPointSampler(ID3D11Device *d3dDev);
	Microsoft::WRL::ComPtr<ID3D11SamplerState> GetLinearSampler(ID3D11Device *d3dDev);
private:
	Microsoft::WRL::ComPtr<ID3D11SamplerState> pointSampler;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> linearSampler;
};