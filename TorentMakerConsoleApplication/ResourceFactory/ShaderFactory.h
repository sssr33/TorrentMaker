#pragma once
#include "..\DxHelpres\DxHelpers.h"
#include "..\Shaders\Yuv420pToRgbaPS.h"

#include <memory>

class ShaderFactory {
public:
	class PS {
	public:
		const std::shared_ptr<Yuv420pToRgbaPS> &GetYuv420pToRgbaPS(ID3D11Device *dev);

	private:
		std::shared_ptr<Yuv420pToRgbaPS> yuv420pToRgbaPS;
	} PS;

};