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
		const MatrixCBuffer &cbuffer);

	static void Set(
		ID3D11DeviceContext *d3dCtx,
		const DynamicGeometry &geom,
		const DynamicGeometryVS &vs,
		const MatrixCBuffer &cbuffer,
		D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
};