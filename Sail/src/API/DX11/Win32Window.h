#pragma once

#include "Sail/api/Window.h"

// Exclude some less used APIs to speed up the build process
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <string>

class Win32Window : public Window {

public:
	Win32Window(const WindowProps& props);
	~Win32Window();

	virtual bool initialize() override;

	LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	
	// NOTE: this method is only used internally by the sail and should not be called by the user
	// Returns true if the window has been resized
	// after the last call of this method
	bool hasBeenResized();

	virtual void setWindowTitle(const std::string& title) override;
	const HWND* getHwnd() const;
	virtual unsigned int getWindowWidth() const override;
	virtual unsigned int getWindowHeight() const override;

private:

private:
	HWND m_hWnd;
	HINSTANCE m_hInstance;
	std::string m_windowTitle;
	DWORD m_windowStyle;
	bool m_resized;

};