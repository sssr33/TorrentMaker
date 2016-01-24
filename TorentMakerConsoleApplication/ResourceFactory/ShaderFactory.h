#pragma once
#include "..\DxHelpres\DxHelpers.h"

#include "..\Shaders\QuadStripFltIndexVs.h"
#include "..\Shaders\DynamicGeometryVS.h"

#include "..\Shaders\Yuv420pToRgbaPS.h"
#include "..\Shaders\ColorPS.h"
#include "..\Shaders\Tex1PS.h"
#include "..\Shaders\Gradient2DPS.h"

#include <memory>
#include <libhelpers\Thread\critical_section.h>

class ShaderFactory {
public:
	class VS {
	public:
		const std::shared_ptr<QuadStripFltIndexVs> &GetQuadStripFltIndexVs(ID3D11Device *dev);
		const std::shared_ptr<DynamicGeometryVS> &GetDynamicGeometryVS(ID3D11Device *dev);

	private:
		thread::critical_section cs;

		std::shared_ptr<QuadStripFltIndexVs> quadStripFltIndexVs;
		std::shared_ptr<DynamicGeometryVS> dynamicGeometryVS;
	} VS;

	class PS {
	public:
		const std::shared_ptr<Yuv420pToRgbaPS> &GetYuv420pToRgbaPS(ID3D11Device *dev);
		const std::shared_ptr<ColorPS> &GetColorPS(ID3D11Device *dev);
		const std::shared_ptr<Tex1PS> &GetTex1PS(ID3D11Device *dev);
		const std::shared_ptr<Gradient2DPS> &GetGradient2DPS(ID3D11Device *dev);

	private:
		thread::critical_section cs;

		std::shared_ptr<Yuv420pToRgbaPS> yuv420pToRgbaPS;
		std::shared_ptr<ColorPS> colorPS;
		std::shared_ptr<Tex1PS> tex1PS;
		std::shared_ptr<Gradient2DPS> gradient2dPS;
	} PS;

};