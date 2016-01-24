#include "ShaderFactory.h"

// VS

const std::shared_ptr<QuadStripFltIndexVs> &ShaderFactory::VS::GetQuadStripFltIndexVs(ID3D11Device *dev) {
	thread::critical_section::scoped_lock lk(this->cs);

	if (!this->quadStripFltIndexVs) {
		this->quadStripFltIndexVs = std::make_shared<QuadStripFltIndexVs>(dev);
	}

	return this->quadStripFltIndexVs;
}

const std::shared_ptr<DynamicGeometryVS> &ShaderFactory::VS::GetDynamicGeometryVS(ID3D11Device *dev) {
	thread::critical_section::scoped_lock lk(this->cs);

	if (!this->dynamicGeometryVS) {
		this->dynamicGeometryVS = std::make_shared<DynamicGeometryVS>(dev);
	}

	return this->dynamicGeometryVS;
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

const std::shared_ptr<Gradient2DPS> &ShaderFactory::PS::GetGradient2DPS(ID3D11Device *dev) {
	thread::critical_section::scoped_lock lk(this->cs);

	if (!this->gradient2dPS) {
		this->gradient2dPS = std::make_shared<Gradient2DPS>(dev);
	}

	return this->gradient2dPS;
}