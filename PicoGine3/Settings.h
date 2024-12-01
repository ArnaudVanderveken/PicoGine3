#ifndef SETTINGS_H
#define SETTINGS_H

#include "WindowFullscreenState.h"

class Settings final : public Singleton<Settings>
{
	friend class Singleton<Settings>;
	explicit Settings();

public:
	~Settings() override = default;

	Settings(const Settings&) noexcept = delete;
	Settings& operator=(const Settings&) noexcept = delete;
	Settings(Settings&&) noexcept = delete;
	Settings& operator=(Settings&&) noexcept = delete;

	[[nodiscard]] bool IsInitialized() const;

	[[nodiscard]] const wchar_t* GetWindowName() const;
	[[nodiscard]] const XMINT2& GetDesiredResolution() const;
	[[nodiscard]] bool GetVSync() const;
	[[nodiscard]] WindowFullscreenState GetWindowFullscreenState() const;

private:
	bool m_IsInitialized;

#if defined(_DX12) && defined(_DEBUG)
	const wchar_t* m_WindowName{ L"PicoGine3 - DEBUG - DX12" };
#elif defined(_DX12) && !defined(_DEBUG)
	const wchar_t* m_WindowName{ L"PicoGine3 - RELEASE - DX12" };
#elif defined(_VK) && defined(_DEBUG)
	const wchar_t* m_WindowName{ L"PicoGine3 - DEBUG - Vulkan" };
#elif defined(_VK) && !defined(_DEBUG)
	const wchar_t* m_WindowName{ L"PicoGine3 - RELEASE - Vulkan" };
#else
	const wchar_t* m_WindowName{ L"ERROR - INVALID API" };
#endif

	XMINT2 m_DesiredResolution;
	bool m_VSync;
	WindowFullscreenState m_WindowFullscreenState;

};

#endif //SETTINGS_H