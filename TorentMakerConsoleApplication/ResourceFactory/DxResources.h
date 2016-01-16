#pragma once
#include "BlendStateFactory.h"
#include "SamplerFactory.h"
#include "ShaderFactory.h"
#include "GeometryFactory.h"
#include "InputAssembler.h"

class DxResources {
public:
	BlendStateFactory Blend;
	SamplerFactory Samplers;
	ShaderFactory Shaders;
	GeometryFactory Geometry;
};