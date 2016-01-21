#include "InputAssembler.h"

void InputAssembler::Set(
	ID3D11DeviceContext *d3dCtx,
	const QuadStripFltIdx &geom,
	const QuadStripFltIndexVs &vs,
	const QuadStripFltIndexVsCBuffer &cbuffer)
{
	auto &inputLayout = vs.GetInputLayout();

	d3dCtx->IASetInputLayout(inputLayout.Get());
	d3dCtx->IASetPrimitiveTopology(geom.GetTopology());
	
	geom.SetToContext(d3dCtx);
	vs.SetToContext(d3dCtx, cbuffer);
}