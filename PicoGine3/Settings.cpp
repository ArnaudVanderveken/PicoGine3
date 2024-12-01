#include "pch.h"
#include "Settings.h"

Settings::Settings() :
	m_IsInitialized{ true },
	m_DesiredResolution{ 1920, 1080 },
	m_VSync{ false },
	m_WindowFullscreenState{ WindowFullscreenState::None }
{
}

bool Settings::IsInitialized() const
{
	return m_IsInitialized;
}

const wchar_t* Settings::GetWindowName() const
{
	return m_WindowName;
}

const XMINT2& Settings::GetDesiredResolution() const
{
	return m_DesiredResolution;
}

bool Settings::GetVSync() const
{
	return m_VSync;
}

WindowFullscreenState Settings::GetWindowFullscreenState() const
{
	return m_WindowFullscreenState;
}
