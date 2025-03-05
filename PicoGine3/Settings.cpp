#include "pch.h"
#include "Settings.h"


void Settings::Initialize()
{
	m_IsInitialized = true;
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

bool Settings::IsVSyncEnabled() const
{
	return m_VSync;
}

bool Settings::IsFrameCapEnabled() const
{
	return m_FrameCap;
}

float Settings::GetMaxFPS() const
{
	return m_MaxFPS;
}

WindowFullscreenState Settings::GetWindowFullscreenStartState() const
{
	return m_WindowFullscreenStartState;
}

void Settings::SetVSync(bool value)
{
	m_VSync = value;
}
