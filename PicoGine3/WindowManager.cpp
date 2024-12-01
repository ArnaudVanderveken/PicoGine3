#include "pch.h"
#include "WindowManager.h"

#include "CoreSystems.h"
#include "Settings.h"


WindowManager::WindowManager():
	m_IsInitialized{ false },
	m_FullscreenState{ WindowFullscreenState::None },
	m_pWindowClassName{ L"PicoGine3WindowClassName" },
	m_WindowHandle{},
	m_WindowRect{}
{
	const auto& core = CoreSystems::Get();
	const auto& settings = Settings::Get();

	//Create Window Class
	WNDCLASS windowClass;
	ZeroMemory(&windowClass, sizeof(WNDCLASS));

	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.hIcon = nullptr;
	windowClass.hbrBackground = nullptr;
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = CoreSystems::WindowProc;
	windowClass.hInstance = core.GetAppHinstance();
	windowClass.lpszClassName = m_pWindowClassName;

	if (!RegisterClass(&windowClass))
		HANDLE_ERROR(HRESULT_FROM_WIN32(GetLastError()))

	//Create Window
	m_WindowRect = { 0, 0, settings.GetDesiredResolution().x, settings.GetDesiredResolution().y };
	AdjustWindowRect(&m_WindowRect, WS_OVERLAPPEDWINDOW, false);
	m_ActualWindowWidth = m_WindowRect.right - m_WindowRect.left;
	m_ActualWindowHeight = m_WindowRect.bottom - m_WindowRect.top;

	m_WindowHandle = CreateWindow(m_pWindowClassName,
		settings.GetWindowName(),
		WS_OVERLAPPEDWINDOW,
		0,
		0,
		m_ActualWindowWidth,
		m_ActualWindowHeight,
		NULL,
		nullptr,
		core.GetAppHinstance(),
		this
	);

	if (!m_WindowHandle)
		HANDLE_ERROR(HRESULT_FROM_WIN32(GetLastError()))

	m_IsInitialized = true;

	//Show The Window
	ShowWindow(m_WindowHandle, SW_SHOWDEFAULT);

	
}

WindowManager::~WindowManager()
{
	DestroyWindow(m_WindowHandle);
	UnregisterClass(m_pWindowClassName, CoreSystems::Get().GetAppHinstance());
}

bool WindowManager::IsInitialized() const
{
	return m_IsInitialized;
}

HWND WindowManager::GetHWnd() const
{
    return m_WindowHandle;
}

void WindowManager::SetFullscreenState(WindowFullscreenState state)
{
	// Moved outside of switch to suppress warning
	static LONG windowStyle{};
	static HMONITOR hMonitor{};
	static MONITORINFOEX monitorInfo{};

	if (state == m_FullscreenState)
		return;

	m_FullscreenState = state;

    switch (state)
    {
    case WindowFullscreenState::None:
		// Store the current window dimensions so they can be restored when switching out of fullscreen state.
		::GetWindowRect(m_WindowHandle, &m_WindowRect);

		// Set the window style to a borderless window so the client area fills the entire screen.
		windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

		::SetWindowLongW(m_WindowHandle, GWL_STYLE, windowStyle);

		// Query the name of the nearest display device for the window.
		// This is required to set the fullscreen dimensions of the window when using a multi-monitor setup.
		hMonitor = ::MonitorFromWindow(m_WindowHandle, MONITOR_DEFAULTTONEAREST);
		
		monitorInfo.cbSize = sizeof(MONITORINFOEX);
		::GetMonitorInfo(hMonitor, &monitorInfo);

		::SetWindowPos(m_WindowHandle, HWND_TOP,
			monitorInfo.rcMonitor.left,
			monitorInfo.rcMonitor.top,
			monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
			monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
			SWP_FRAMECHANGED | SWP_NOACTIVATE);

		::ShowWindow(m_WindowHandle, SW_MAXIMIZE);
	    break;

    case WindowFullscreenState::Borderless:
		// Restore all the window decorators.
		::SetWindowLong(m_WindowHandle, GWL_STYLE, WS_OVERLAPPEDWINDOW);

		::SetWindowPos(m_WindowHandle, HWND_NOTOPMOST,
			m_WindowRect.left,
			m_WindowRect.top,
			m_WindowRect.right - m_WindowRect.left,
			m_WindowRect.bottom - m_WindowRect.top,
			SWP_FRAMECHANGED | SWP_NOACTIVATE);

		::ShowWindow(m_WindowHandle, SW_NORMAL);
	    break;

    /*case WindowFullscreenState::Fullscreen:
    	break;*/
    }
}

WindowFullscreenState WindowManager::GetFullscreenState() const
{
	return m_FullscreenState;
}

void WindowManager::UpdateWindowRect(RECT rect)
{
	if (!m_IsInitialized)
		return;

	m_WindowRect = rect;
	m_ActualWindowWidth = m_WindowRect.right - m_WindowRect.left;
	m_ActualWindowHeight = m_WindowRect.bottom - m_WindowRect.top;
}
