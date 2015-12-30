#pragma once
#include "..\DxHelpres\DxHelpers.h"

class ShaderCBuffer {
public:
	ShaderCBuffer(ID3D11Device *d3dDev, uint32_t size);

	const Microsoft::WRL::ComPtr<ID3D11Buffer> &GetBuffer() const;

protected:
	template<class T>
	void Update(ID3D11DeviceContext *d3dCtx, T data) {
		d3dCtx->UpdateSubresource(this->buffer.Get(), 0, nullptr, &data, 0, 0);
	}

	template<class T>
	void Update(ID3D11DeviceContext *d3dCtx, T *data) {
		d3dCtx->UpdateSubresource(this->buffer.Get(), 0, nullptr, data, 0, 0);
	}

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
};