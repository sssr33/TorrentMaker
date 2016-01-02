#include "DxDevice.h"

#include <libhelpers\H.h>

DxDevice::DxDevice()
	: featureLevel(D3D_FEATURE_LEVEL_9_1)
{
	this->CreateDeviceIndependentResources();
	this->CreateDeviceDependentResources();
}

DxDevice::~DxDevice() {
}

IDWriteFactory *DxDevice::GetDwriteFactory() const {
	return this->dwriteFactory.Get();
}

ID2D1Factory1 *DxDevice::GetD2D1Factory() const {
	return this->d2dFactory.Get();
}

ID3D11Device *DxDevice::GetD3DDevice() const {
	return this->d3dDev.Get();
}

ID2D1Device *DxDevice::GetD2DDevice() const {
	return this->d2dDevice.Get();
}

critical_section_guard<DxDeviceCtx>::Accessor DxDevice::GetContext() {
	return this->ctx.Get();
}

void DxDevice::CreateDeviceIndependentResources() {
	HRESULT hr = S_OK;

	hr = DWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(IDWriteFactory),
		(IUnknown **)this->dwriteFactory.GetAddressOf());
	H::System::ThrowIfFailed(hr);

	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, IID_PPV_ARGS(this->d2dFactory.GetAddressOf()));
	H::System::ThrowIfFailed(hr);
}

void DxDevice::CreateDeviceDependentResources() {
	HRESULT hr = S_OK;
	uint32_t flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3dCtx;
	Microsoft::WRL::ComPtr<ID2D1DeviceContext> d2dCtx;

#ifdef _DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE,
		nullptr, flags,
		featureLevels, ARRAY_SIZE(featureLevels),
		D3D11_SDK_VERSION,
		this->d3dDev.GetAddressOf(), &this->featureLevel,
		d3dCtx.GetAddressOf());

	if (FAILED(hr)) {
		hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP,
			nullptr, flags,
			featureLevels, ARRAY_SIZE(featureLevels),
			D3D11_SDK_VERSION,
			this->d3dDev.GetAddressOf(), &this->featureLevel,
			d3dCtx.GetAddressOf());
		H::System::ThrowIfFailed(hr);
	}

	this->EnableD3DDeviceMultithreading();
	this->CreateD2DDevice();
	d2dCtx = this->CreateD2DDeviceContext();

	auto ctxAcc = this->ctx.Get();
	*ctxAcc = DxDeviceCtx(d3dCtx, d2dCtx);
}

void DxDevice::EnableD3DDeviceMultithreading() {
	HRESULT hr = S_OK;
	Microsoft::WRL::ComPtr<ID3D10Multithread> multithread;

	hr = this->d3dDev.As(&multithread);
	H::System::ThrowIfFailed(hr);

	multithread->SetMultithreadProtected(TRUE);
}

void DxDevice::CreateD2DDevice() {
	HRESULT hr = S_OK;
	D2D1_CREATION_PROPERTIES creationProps;
	Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDev;

	hr = this->d3dDev.As(&dxgiDev);
	H::System::ThrowIfFailed(hr);

#ifdef _DEBUG
	creationProps.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#else
	creationProps.debugLevel = D2D1_DEBUG_LEVEL_NONE;
#endif

	creationProps.options = D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS;
	creationProps.threadingMode = D2D1_THREADING_MODE_MULTI_THREADED;

	/*
	https://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=EN-US&k=k%28d2d1_1%2FD2D1CreateDevice%29;k%28D2D1CreateDevice%29;k%28DevLang-C%2B%2B%29;k%28TargetOS-Windows%29&rd=true
	It's probably better to use D2DFactory::CreateDevice
	to use same d2dFactory
	*/
	hr = this->d2dFactory->CreateDevice(dxgiDev.Get(), this->d2dDevice.GetAddressOf());
	H::System::ThrowIfFailed(hr);

	/*hr = D2D1CreateDevice(dxgiDev.Get(), creationProps, this->d2dDevice.GetAddressOf());
	H::System::ThrowIfFailed(hr);*/
}

Microsoft::WRL::ComPtr<ID2D1DeviceContext> DxDevice::CreateD2DDeviceContext() {
	HRESULT hr = S_OK;
	Microsoft::WRL::ComPtr<ID2D1DeviceContext> d2dCtx;

	hr = this->d2dDevice->CreateDeviceContext(
		D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS,
		d2dCtx.GetAddressOf());
	H::System::ThrowIfFailed(hr);

	return d2dCtx;
}