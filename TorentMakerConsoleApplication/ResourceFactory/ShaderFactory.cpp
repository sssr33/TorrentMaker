#include "ShaderFactory.h"

// VS

const std::shared_ptr<QuadStripFltIndexVs> &ShaderFactory::VS::GetQuadStripFltIndexVs(ID3D11Device *dev) {
	thread::critical_section::scoped_lock lk(this->cs);

	if (!this->quadStripFltIndexVs) {
		this->quadStripFltIndexVs = std::make_shared<QuadStripFltIndexVs>(dev);
	}

	return this->quadStripFltIndexVs;
}




// PS

const std::shared_ptr<Yuv420pToRgbaPS> &ShaderFactory::PS::GetYuv420pToRgbaPS(ID3D11Device *dev) {
	thread::critical_section::scoped_lock lk(this->cs);

	if (!this->yuv420pToRgbaPS) {
		this->yuv420pToRgbaPS = std::make_shared<Yuv420pToRgbaPS>(dev);
	}

	return this->yuv420pToRgbaPS;
}

const std::shared_ptr<ColorPS> &ShaderFactory::PS::GetColorPS(ID3D11Device *dev) {
	thread::critical_section::scoped_lock lk(this->cs);

	if (!this->colorPS) {
		this->colorPS = std::make_shared<ColorPS>(dev);
	}

	return this->colorPS;
}

const std::shared_ptr<Tex1PS> &ShaderFactory::PS::GetTex1PS(ID3D11Device *dev) {
	thread::critical_section::scoped_lock lk(this->cs);

	if (!this->tex1PS) {
		this->tex1PS = std::make_shared<Tex1PS>(dev);
	}

	return this->tex1PS;
}