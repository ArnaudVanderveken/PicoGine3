#pragma once

#include <sdl.h>

class WindowManager final
{
public:
	enum class WindowFullscreenState
	{
		None,
		Borderless,
		Fullscreen
	};

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

	void SetFullscreenState(WindowFullscreenState state);
	[[nodiscard]] WindowFullscreenState GetFullscreenState() const;

private:

#if defined(_DX12)
	#if defined(_DEBUG)
		const char* m_WindowName{ "PicoGine3 - DEBUG - DX12" };
	#else
		const char* m_WindowName{ "PicoGine3 - RELEASE - DX12" };
	#endif

#elif defined(_VK)
	#if defined(_DEBUG)
		const char* m_WindowName{ "PicoGine3 - DEBUG - Vulkan" };
	#else
		const char* m_WindowName{ "PicoGine3 - RELEASE - Vulkan" };
	#endif

#else
	const char* m_WindowName{ "ERROR - INVALID API" };
#endif

	bool m_IsInitialized;

	SDL_Window* m_pWindow;
	HINSTANCE m_HInstance;
	HWND m_WindowHandle;

};

