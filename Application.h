#pragma once

#include "Graphics.h"
#include "Renderer.h"
#include "DeviceManager.h"

class Application
{
public:
	Application(void) = delete;
	~Application(void) = delete;

	static bool Initialize(HINSTANCE instance) noexcept;
	static void Finish(void) noexcept;

	static void Start(void) noexcept;
	static void Stop(void) noexcept;

	static HINSTANCE WinAppInstance(void) noexcept;
	static HWND WindowHandle(void) noexcept;
	static Graphics* GetGraphics(void) noexcept;

protected:
private:

};
