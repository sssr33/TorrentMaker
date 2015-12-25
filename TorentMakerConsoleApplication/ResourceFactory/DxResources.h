#pragma once
#include "SamplerFactory.h"
#include "ShaderFactory.h"

class DxResources {
public:
	SamplerFactory Samplers;
	ShaderFactory Shaders;
};