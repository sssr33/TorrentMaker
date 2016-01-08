#include "Texture2D.h"

#include <array>
#include <libhelpers\H.h>

Texture2D::~Texture2D() {
}

ID3D11Texture2D *Texture2D::GetTexture(uint32_t plane) const {
	return this->tex[plane];
}

DirectX::XMUINT2 Texture2D::GetSize(uint32_t plane) const {
	D3D11_TEXTURE2D_DESC desc;
	auto texTmp = this->tex[plane];

	texTmp->GetDesc(&desc);

	return DirectX::XMUINT2(desc.Width, desc.Height);
}

size_t Texture2D::GetPlaneCount() const {
	return this->tex.size();
}

void Texture2D::AddPlane(ID3D11Texture2D *tex) {
	this->tex.push_back(tex);
}

void Texture2D::AddPlane(const Microsoft::WRL::ComPtr<ID3D11Texture2D> &tex) {
	this->tex.push_back(tex);
}




Texture2DResource::~Texture2DResource() {
}

ID3D11ShaderResourceView *Texture2DResource::GetShaderResourceView(uint32_t plane) const {
	return this->srv[plane];
}

void Texture2DResource::SetToContextPS(ID3D11DeviceContext *d3dCtx, uint32_t startSlot) const {
	d3dCtx->PSSetShaderResources(startSlot, (uint32_t)this->srv.size(), this->srv.data());
}

void Texture2DResource::AddPlane(ID3D11Texture2D *tex, ID3D11ShaderResourceView *srv) {
	Texture2D::AddPlane(tex);
	this->srv.push_back(srv);
}

void Texture2DResource::AddPlane(
	const Microsoft::WRL::ComPtr<ID3D11Texture2D> &tex,
	const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> &srv)
{
	Texture2D::AddPlane(tex);
	this->srv.push_back(srv);
}




Texture2DRenderTarget::~Texture2DRenderTarget() {
}

ID3D11RenderTargetView *Texture2DRenderTarget::GetRenderTargetView(uint32_t plane) const {
	return this->rtv[plane];
}

void Texture2DRenderTarget::Clear(ID3D11DeviceContext *d3dCtx, const float color[4]) const {
	for (auto &i : this->rtv) {
		d3dCtx->ClearRenderTargetView(i, color);
	}
}

void Texture2DRenderTarget::SetRenderTarget(ID3D11DeviceContext *d3dCtx) const {
	d3dCtx->OMSetRenderTargets((uint32_t)this->rtv.size(), this->rtv.data(), nullptr);
}

void Texture2DRenderTarget::SetViewport(ID3D11DeviceContext *d3dCtx) const {
	std::vector<D3D11_VIEWPORT> viewports;

	viewports.reserve(this->GetPlaneCount());

	for (uint32_t i = 0; i < this->GetPlaneCount(); i++) {
		D3D11_VIEWPORT viewport;
		auto size = this->GetSize(i);

		viewport.Width = (float)size.x;
		viewport.Height = (float)size.y;
		viewport.TopLeftX = viewport.TopLeftY = 0.0f;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		viewports.push_back(viewport);
	}

	d3dCtx->RSSetViewports((uint32_t)viewports.size(), viewports.data());
}

void Texture2DRenderTarget::AddPlane(ID3D11Texture2D *tex, ID3D11ShaderResourceView *srv, ID3D11RenderTargetView *rtv) {
	Texture2DResource::AddPlane(tex, srv);
	this->rtv.push_back(rtv);
}

void Texture2DRenderTarget::AddPlane(
	const Microsoft::WRL::ComPtr<ID3D11Texture2D> &tex,
	const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> &srv,
	const Microsoft::WRL::ComPtr<ID3D11RenderTargetView> &rtv)
{
	Texture2DResource::AddPlane(tex, srv);
	this->rtv.push_back(rtv);
}




Texture2DRenderTargetWithD2D::~Texture2DRenderTargetWithD2D() {
}

ID2D1Bitmap1 *Texture2DRenderTargetWithD2D::GetD2DBitmap(uint32_t plane) const {
	return this->d2dBitmap[plane];
}

ID2D1BitmapBrush1 *Texture2DRenderTargetWithD2D::GetD2DBitmapBrush(uint32_t plane) const {
	return this->d2dBitmapBrush[plane];
}

void Texture2DRenderTargetWithD2D::SetD2DRenderTarget(ID2D1DeviceContext *d2dCtx, uint32_t plane, float dpiX, float dpiY) const {
	d2dCtx->SetTarget(this->d2dBitmap[plane]);
	d2dCtx->SetDpi(dpiX, dpiY);
}

void Texture2DRenderTargetWithD2D::AddPlane(
	ID3D11Texture2D *tex,
	ID3D11ShaderResourceView *srv,
	ID3D11RenderTargetView *rtv,
	ID2D1Bitmap1 *d2dBmp,
	ID2D1BitmapBrush1 *d2dBmpBrush)
{
	Texture2DRenderTarget::AddPlane(tex, srv, rtv);
	this->d2dBitmap.push_back(d2dBmp);
	this->d2dBitmapBrush.push_back(d2dBmpBrush);
}

void Texture2DRenderTargetWithD2D::AddPlane(
	const Microsoft::WRL::ComPtr<ID3D11Texture2D> &tex,
	const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> &srv,
	const Microsoft::WRL::ComPtr<ID3D11RenderTargetView> &rtv,
	const Microsoft::WRL::ComPtr<ID2D1Bitmap1> &d2dBmp,
	const Microsoft::WRL::ComPtr<ID2D1BitmapBrush1> &d2dBmpBrush)
{
	Texture2DRenderTarget::AddPlane(tex, srv, rtv);
	this->d2dBitmap.push_back(d2dBmp);
	this->d2dBitmapBrush.push_back(d2dBmpBrush);
}