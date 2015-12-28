#pragma once
#include "..\DxHelpres\DxDevice.h"
#include "TextureCpuAccess.h"

#include <libhelpers\Containers\comptr_vector.h>

// TODO refactoring for textures with static count of planes
class Texture2D {
public:
	virtual ~Texture2D();

	ID3D11Texture2D *GetTexture(uint32_t plane = 0) const;
	DirectX::XMUINT2 GetSize(uint32_t plane = 0) const;
	size_t GetPlaneCount() const;

protected:
	void AddPlane(ID3D11Texture2D *tex);
	void AddPlane(const Microsoft::WRL::ComPtr<ID3D11Texture2D> &tex);
private:
	comptr_vector<ID3D11Texture2D> tex;
};




class Texture2DResource : public Texture2D {
public:
	virtual ~Texture2DResource();

	ID3D11ShaderResourceView *GetShaderResourceView(uint32_t plane = 0) const;

	void SetToContextPS(ID3D11DeviceContext *d3dCtx, uint32_t startSlot = 0) const;

protected:
	void AddPlane(ID3D11Texture2D *tex, ID3D11ShaderResourceView *srv);
	void AddPlane(
		const Microsoft::WRL::ComPtr<ID3D11Texture2D> &tex, 
		const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> &srv);

private:
	comptr_vector<ID3D11ShaderResourceView> srv;
};




class Texture2DRenderTarget : public Texture2DResource {
public:
	virtual ~Texture2DRenderTarget();

	ID3D11RenderTargetView *GetRenderTargetView(uint32_t plane = 0) const;

	void Clear(ID3D11DeviceContext *d3dCtx, const float color[4] = DirectX::Colors::Black) const;
	void SetRenderTarget(ID3D11DeviceContext *d3dCtx) const;
	// TODO refactoring for textures with static count of planes
	void SetViewport(ID3D11DeviceContext *d3dCtx) const;

protected:
	void AddPlane(ID3D11Texture2D *tex, ID3D11ShaderResourceView *srv, ID3D11RenderTargetView *rtv);
	void AddPlane(
		const Microsoft::WRL::ComPtr<ID3D11Texture2D> &tex,
		const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> &srv,
		const Microsoft::WRL::ComPtr<ID3D11RenderTargetView> &rtv);
private:
	comptr_vector<ID3D11RenderTargetView> rtv;
};