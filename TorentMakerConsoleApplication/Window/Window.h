#pragma once

#include <map>
#include <atomic>
#include <libhelpers\H.h>
#include <libhelpers\Thread\critical_section.h>

class Window {
public:
	Window();
	~Window();

private:
	HWND handle;
	std::wstring className;

	LRESULT WndProc(uint32_t msg, WPARAM wparam, LPARAM lparam);

	static std::atomic_uint64_t nextWndId;
	// It should be enough to have 1 CS for all wnd's
	// Also SetWindowLongPtr should NOT be used since :
	// http://stackoverflow.com/questions/117792/best-method-for-storing-this-pointer-for-use-in-wndproc
	static thread::critical_section thisMapCs;
	static std::map<HWND, Window *> thisMap;

	static LRESULT CALLBACK WndProcTmp(HWND h, UINT msg, WPARAM wparam, LPARAM lparam);
	static void AddThisMap(HWND h, Window *_this);
	static Window *GetThisMap(HWND h);
	static void RemoveThisMap(HWND h);
};