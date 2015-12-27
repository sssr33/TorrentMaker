#include "ShaderFactory.h"

// VS

const std::shared_ptr<QuadStripFltIndexVs> &ShaderFactory::VS::GetQuadStripFltIndexVs(ID3D11Device *dev) {
	if (!this->quadStripFltIndexVs) {
		this->quadStripFltIndexVs = std::make_shared<QuadStripFltIndexVs>(dev);
	}

	return this->quadStripFltIndexVs;
}




// PS

const std::shared_ptr<Yuv420pToRgbaPS> &ShaderFactory::PS::GetYuv420pToRgbaPS(ID3D11Device *dev) {
	if (!this->yuv420pToRgbaPS) {
		this->yuv420pToRgbaPS = std::make_shared<Yuv420pToRgbaPS>(dev);
	}

	return this->yuv420pToRgbaPS;
}