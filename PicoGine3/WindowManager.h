#pragma once

#include <sdl.h>

class WindowManager final
{
public:
	explicit WindowManager();
	~WindowManager();

	WindowManager(const WindowManager&) noexcept = delete;
	WindowManager& operator=(const WindowManager&) noexcept = delete;
	WindowManager(WindowManager&&) noexcept = delete;
	WindowManager& operator=(WindowManager&&) noexcept = delete;

	[[nodiscard]] bool IsInitialized() const;

	[[nodiscard]] SDL_Window* GetWindow() const;
	[[nodiscard]] HINSTANCE GetHInstance() const;
	[[nodiscard]] HWND GetHWnd() const;

private:

#if defined(_DX12)
	const char* m_WindowName{ "PicoGine3 - DX12" };
#elif defined(_VK)
	const char* m_WindowName{ "PicoGine3 - Vulkan" };
#else
	const char* m_WindowName{ "ERROR - INVALID API" };
#endif

	bool m_IsInitialized;

	SDL_Window* m_pWindow;
	HINSTANCE m_HInstance;
	HWND m_WindowHandle;

};

