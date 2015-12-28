#pragma once
#include "..\DxHelpres\DxHelpers.h"

#include <libhelpers\H.h>

struct TextureUpdaterData {
	const void *data;
	uint32_t rowPitch;
	uint32_t depthPitch;

	TextureUpdaterData(const void *data, uint32_t rowPitch)
		: data(data), rowPitch(rowPitch), depthPitch(0) {
	}

	TextureUpdaterData(const D3D11_SUBRESOURCE_DATA &data)
		: data(data.pSysMem), rowPitch(data.SysMemPitch), depthPitch(data.SysMemSlicePitch) {
	}
};

template<D3D11_USAGE usage>
class TextureUpdater {
public:
	static void Update(ID3D11DeviceContext *d3dCtx, ID3D11Texture2D *res, const TextureUpdaterData &data) {
		static_assert(false, "Not implemented.");
	}
};

template<>
class TextureUpdater<D3D11_USAGE_DEFAULT> {
public:
	static void Update(ID3D11DeviceContext *d3dCtx, ID3D11Texture2D *res, const TextureUpdaterData &data) {
		d3dCtx->UpdateSubresource(res, 0, nullptr, data.data, data.rowPitch, data.depthPitch);
	}
};

template<>
class TextureUpdater<D3D11_USAGE_DYNAMIC> {
public:
	static void Update(ID3D11DeviceContext *d3dCtx, ID3D11Texture2D *res, const TextureUpdaterData &data) {
		D3D11_TEXTURE2D_DESC desc;
		auto &src = data;
		MappedResource mapped(d3dCtx, res, D3D11_MAP_WRITE_DISCARD);

		res->GetDesc(&desc);

		if (src.rowPitch == mapped.GetRowPitch()) {
			auto copySize = src.rowPitch * desc.Height;

			std::memcpy(mapped.GetData(), src.data, copySize);
		}
		else {
			auto rowSize = (std::min)(src.rowPitch, mapped.GetRowPitch());
			auto srcTmp = reinterpret_cast<const uint8_t *>(src.data);
			auto dstTmp = reinterpret_cast<uint8_t *>(mapped.GetData());

			for (uint32_t y = 0; y < desc.Height; y++, srcTmp += src.rowPitch, dstTmp += mapped.GetRowPitch()) {
				std::memcpy(dstTmp, srcTmp, rowSize);
			}
		}
	}
};