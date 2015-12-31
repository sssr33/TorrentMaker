#include "SimpleDxWindow.h"

SimpleDxWindow::RenderScope::RenderScope(SimpleDxWindow *window, RenderTargetState<1> &&state)
	: window(window), state(std::move(state)) {
}

SimpleDxWindow::RenderScope::RenderScope(RenderScope &&other)
	: window(std::move(other.window)), state(std::move(other.state)) 
{
	other.window = nullptr;
}

SimpleDxWindow::RenderScope::~RenderScope() {
	if (this->window) {
		this->window->Present();
	}
}

SimpleDxWindow::RenderScope &SimpleDxWindow::RenderScope::operator=(RenderScope &&other) {
	if (this != &other) {
		this->window = std::move(other.window);
		this->state = std::move(other.state);

		other.window = nullptr;
	}

	return *this;
}




SimpleDxWindow::SimpleDxWindow(DxDevice &dxDev)
	: DxWindow(dxDev)
{
	this->Show();
}

SimpleDxWindow::~SimpleDxWindow() {
}

void SimpleDxWindow::SetOnSizeChanged(std::function<void(const DirectX::XMUINT2 &newSize)> v) {
	this->onSizeChanged = v;

	if (this->onSizeChanged) {
		this->onSizeChanged(this->GetOutputSize());
	}
}

SimpleDxWindow::RenderScope SimpleDxWindow::Begin(ID3D11DeviceContext *d3dCtx, const float color[4]) {
	this->ProcessMessages();
	this->Clear(d3dCtx, DirectX::Colors::CornflowerBlue);
	auto state = this->SetToContext(d3dCtx);

	return SimpleDxWindow::RenderScope(this, std::move(state));
}

void SimpleDxWindow::CreateSizeDependentResources(const DirectX::XMUINT2 &newSize) {
	if (this->onSizeChanged) {
		this->onSizeChanged(newSize);
	}
}