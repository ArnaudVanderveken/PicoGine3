#pragma once

#include "WindowFullscreenState.h"

class WindowManager final : public Singleton<WindowManager>
{
	friend class Singleton<WindowManager>;
	explicit WindowManager();

public:
	~WindowManager() override;

	WindowManager(const WindowManager&) noexcept = delete;
	WindowManager& operator=(const WindowManager&) noexcept = delete;
	WindowManager(WindowManager&&) noexcept = delete;
	WindowManager& operator=(WindowManager&&) noexcept = delete;

	[[nodiscard]] bool IsInitialized() const;

	[[nodiscard]] HWND GetHWnd() const;

	void SetFullscreenState(WindowFullscreenState state);
	[[nodiscard]] WindowFullscreenState GetFullscreenState() const;

	void UpdateWindowRect(RECT rect);

private:
	bool m_IsInitialized;

	WindowFullscreenState m_FullscreenState;

	const wchar_t* m_pWindowClassName;
	HWND m_WindowHandle;
	RECT m_WindowRect;
	int m_ActualWindowWidth;
	int m_ActualWindowHeight;
};

