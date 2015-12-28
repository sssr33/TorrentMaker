#pragma once
#include "..\DxHelpres\DxHelpers.h"

template<D3D11_USAGE>
struct TextureCpuAccess {
	static D3D11_CPU_ACCESS_FLAG Get() {
		return (D3D11_CPU_ACCESS_FLAG)0;
	}
};

template<>
struct TextureCpuAccess<D3D11_USAGE_DYNAMIC> {
	static D3D11_CPU_ACCESS_FLAG Get() {
		return D3D11_CPU_ACCESS_WRITE;
	}
};

template<>
struct TextureCpuAccess<D3D11_USAGE_STAGING> {
	static D3D11_CPU_ACCESS_FLAG Get() {
		const auto tmp = (D3D11_CPU_ACCESS_FLAG)(D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE);
		return tmp;
	}
};