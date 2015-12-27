#pragma once
#include "..\DxHelpres\DxHelpers.h"
#include "..\Geometry\QuadStripFltIdx.h"

class GeometryFactory {
public:
	const std::shared_ptr<QuadStripFltIdx> &GetQuadStripIdx(ID3D11Device *dev);

private:
	std::shared_ptr<QuadStripFltIdx> quadStripIdx;
};