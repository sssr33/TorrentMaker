#pragma once
#include "..\DxHelpres\DxHelpers.h"
#include "..\Geometry\QuadStripFltIdx.h"
#include "..\Geometry\DynamicGeometry.h"

#include <libhelpers\Thread\critical_section.h>

class GeometryFactory {
public:
	const std::shared_ptr<QuadStripFltIdx> &GetQuadStripIdx(ID3D11Device *dev);

private:
	thread::critical_section cs;

	std::shared_ptr<QuadStripFltIdx> quadStripIdx;
};