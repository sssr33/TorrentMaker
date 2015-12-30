#include "Window.h"

Window::Window()
	: handle(NULL) 
{
	WNDCLASSW wndclass;
	auto hinst = GetModuleHandleW(NULL);
	this->className = L"Window" + std::to_wstring(Window::nextWndId++);

	wndclass.style = CS_DBLCLKS;
	wndclass.lpfnWndProc = Window::WndProcTmp;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hinst;
	wndclass.hIcon = NULL;
	wndclass.hCursor = NULL;
	wndclass.hbrBackground = HBRUSH(COLOR_WINDOW + 1);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = this->className.data();

	if (!RegisterClassW(&wndclass)) {
		H::System::ThrowIfFailed(E_FAIL);
	}
	
	this->handle = CreateWindowW(
		this->className.data(),
		L"WindowTitle",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		hinst,
		NULL);

	if (!this->handle) {
		H::System::ThrowIfFailed(E_FAIL);
	}

	Window::AddThisMap(this->handle, this);
}

Window::~Window() {
	DestroyWindow(this->handle);
	Window::RemoveThisMap(this->handle);
}

DirectX::XMUINT2 Window::GetSize() const {
	RECT rect;
	DirectX::XMUINT2 size;

	if (GetClientRect(this->handle, &rect)) {
		size.x = rect.right - rect.left;
		size.y = rect.bottom - rect.top;
	}
	else {
		size.x = size.y = 1;
	}

	return size;
}

void Window::Show() {
	ShowWindow(this->handle, SW_SHOWDEFAULT);
}

void Window::ProcessMessages() {
	MSG msg;

	while (PeekMessageW(&msg, this->handle, 0, 0, PM_REMOVE)) {
		DispatchMessageW(&msg);
	}
}

HWND Window::GetHwnd() const {
	return this->handle;
}

const std::wstring &Window::GetWndClassName() const {
	return this->className;
}

LRESULT Window::WndProc(uint32_t msg, WPARAM wparam, LPARAM lparam) {
	this->ProcessMsg(msg, wparam, lparam);

	switch (msg) {
	case WM_DESTROY:
		// no PostQuitMessage(0) since want to continue run the app
		// and looks like that there is no mem. leak
		// TODO make more tests for mem leak on wnd destroy.
		return 0L;
	default:
		return DefWindowProc(this->handle, msg, wparam, lparam);
	}
}




std::atomic_uint64_t Window::nextWndId(0);
thread::critical_section Window::thisMapCs;
std::map<HWND, Window *> Window::thisMap;

LRESULT CALLBACK Window::WndProcTmp(HWND h, UINT msg, WPARAM wparam, LPARAM lparam) {
	auto _this = Window::GetThisMap(h);

	if (!_this) {
		// This cas happen while window is initializing
		return DefWindowProc(h, msg, wparam, lparam);
	}

	// should be alway true
	H::System::Assert(h == _this->handle);

	return _this->WndProc(msg, wparam, lparam);
}

void Window::AddThisMap(HWND h, Window *_this) {
	thread::critical_section::scoped_lock lk(Window::thisMapCs);
	Window::thisMap[h] = _this;
}

Window *Window::GetThisMap(HWND h) {
	thread::critical_section::scoped_lock lk(Window::thisMapCs);
	return Window::thisMap[h];
}

void Window::RemoveThisMap(HWND h) {
	thread::critical_section::scoped_lock lk(Window::thisMapCs);
	Window::thisMap.erase(h);
}