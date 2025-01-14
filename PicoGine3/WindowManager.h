#pragma once

#include "WindowFullscreenState.h"

class WindowManager final : public Singleton<WindowManager>
{
	friend class Singleton<WindowManager>;
	explicit WindowManager() = default;

public:
	~WindowManager() override;

	WindowManager(const WindowManager&) noexcept = delete;
	WindowManager& operator=(const WindowManager&) noexcept = delete;
	WindowManager(WindowManager&&) noexcept = delete;
	WindowManager& operator=(WindowManager&&) noexcept = delete;

	void Initialize();
	[[nodiscard]] bool IsInitialized() const;

	[[nodiscard]] HWND GetHWnd() const;

	void SetFullscreenState(WindowFullscreenState state);
	[[nodiscard]] WindowFullscreenState GetFullscreenState() const;

	void GetWindowDimensions(int& width, int& height) const;

private:
	bool m_IsInitialized{};

	WindowFullscreenState m_FullscreenState;

	const wchar_t* m_pWindowClassName{ L"PicoGine3WindowClassName" };
	HWND m_WindowHandle;
	RECT m_WindowRect;
	int m_ActualWindowWidth;
	int m_ActualWindowHeight;

	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

