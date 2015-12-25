#include "ShaderFactory.h"

const std::shared_ptr<Yuv420pToRgbaPS> &ShaderFactory::PS::GetYuv420pToRgbaPS(ID3D11Device *dev) {
	if (!this->yuv420pToRgbaPS) {
		this->yuv420pToRgbaPS = std::make_shared<Yuv420pToRgbaPS>(dev);
	}

	return this->yuv420pToRgbaPS;
}