#include "InputAssembler.h"

void InputAssembler::Set(
	ID3D11DeviceContext *d3dCtx,
	const QuadStripFltIdx &geom,
	const QuadStripFltIndexVs &vs,
	const MatrixCBuffer &cbuffer)
{
	auto &inputLayout = vs.GetInputLayout();

	d3dCtx->IASetInputLayout(inputLayout.Get());
	d3dCtx->IASetPrimitiveTopology(geom.GetTopology());
	
	geom.SetToContext(d3dCtx);
	vs.SetToContext(d3dCtx, cbuffer);
}

void InputAssembler::Set(
	ID3D11DeviceContext *d3dCtx,
	const DynamicGeometry &geom,
	const DynamicGeometryVS &vs,
	const MatrixCBuffer &cbuffer,
	D3D11_PRIMITIVE_TOPOLOGY topology)
{
	auto &inputLayout = vs.GetInputLayout();

	d3dCtx->IASetInputLayout(inputLayout.Get());
	d3dCtx->IASetPrimitiveTopology(topology);

	geom.SetToContext(d3dCtx);
	vs.SetToContext(d3dCtx, cbuffer);
}