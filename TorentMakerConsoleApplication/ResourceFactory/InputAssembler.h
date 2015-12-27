#pragma once
#include "..\DxHelpres\DxHelpers.h"
#include "GeometryFactory.h"
#include "ShaderFactory.h"

class InputAssembler {
public:

	static void Set(
		ID3D11DeviceContext *d3dCtx, 
		const QuadStripFltIdx &geom, 
		const QuadStripFltIndexVs &vs, 
		const QuadStripFltIndexVsCBuffer &cbuffer);
};