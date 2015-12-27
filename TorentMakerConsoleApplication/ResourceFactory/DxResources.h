#pragma once
#include "SamplerFactory.h"
#include "ShaderFactory.h"
#include "GeometryFactory.h"
#include "InputAssembler.h"

class DxResources {
public:
	SamplerFactory Samplers;
	ShaderFactory Shaders;
	GeometryFactory Geometry;
};