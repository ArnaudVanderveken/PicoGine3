#include "pch.h"
#include "WindowManager.h"

#include "CoreSystems.h"
#include "InputManager.h"
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
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = core.GetAppHinstance();
	windowClass.lpszClassName = m_pWindowClassName;

	if (!::RegisterClass(&windowClass))
		HANDLE_ERROR(::HRESULT_FROM_WIN32(::GetLastError()))

	//Create Window
	m_WindowRect = { 0, 0, settings.GetDesiredResolution().x, settings.GetDesiredResolution().y };
	::AdjustWindowRect(&m_WindowRect, WS_OVERLAPPEDWINDOW, false);
	const auto actualWindowWidth = m_WindowRect.right - m_WindowRect.left;
	const auto actualWindowHeight = m_WindowRect.bottom - m_WindowRect.top;

	m_WindowHandle = CreateWindow(m_pWindowClassName,
		settings.GetWindowName(),
		WS_OVERLAPPEDWINDOW,
		0,
		0,
		actualWindowWidth,
		actualWindowHeight,
		NULL,
		nullptr,
		core.GetAppHinstance(),
		this
	);

	if (!m_WindowHandle)
		HANDLE_ERROR(::HRESULT_FROM_WIN32(::GetLastError()))

	// Center window
	// Query the name of the nearest display device for the window.
	const HMONITOR hMonitor { ::MonitorFromWindow(m_WindowHandle, MONITOR_DEFAULTTONEAREST) };
	MONITORINFOEX monitorInfo{};
	monitorInfo.cbSize = sizeof(MONITORINFOEX);
	::GetMonitorInfo(hMonitor, &monitorInfo);

	const auto left = (monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left) / 2 - (settings.GetDesiredResolution().x / 2);
	const auto top = (monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top) / 2 - (settings.GetDesiredResolution().y / 2);

	::SetWindowPos(m_WindowHandle, nullptr, left, top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

	//Show The Window
	::ShowWindow(m_WindowHandle, SW_SHOWDEFAULT);

	m_IsInitialized = true;
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
	if (state == m_FullscreenState)
		return;

	m_FullscreenState = state;

    switch (state)
    {
	case WindowFullscreenState::Borderless:
	{
		// Store the current window dimensions so they can be restored when switching out of fullscreen state.
		::GetWindowRect(m_WindowHandle, &m_WindowRect);

		// Set the window style to a borderless window so the client area fills the entire screen.
		constexpr LONG windowStyle{ WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX) };

		::SetWindowLongW(m_WindowHandle, GWL_STYLE, windowStyle);

		// Query the name of the nearest display device for the window.
		// This is required to set the fullscreen dimensions of the window when using a multi-monitor setup.
		const HMONITOR hMonitor{ ::MonitorFromWindow(m_WindowHandle, MONITOR_DEFAULTTONEAREST) };

		MONITORINFOEX monitorInfo{};
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
	}
    case WindowFullscreenState::None:
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

LRESULT WindowManager::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;

	case WM_INPUT:
		InputManager::Get().ProcessRawInput(reinterpret_cast<HRAWINPUT>(lParam));
		break;

	case WM_SYSCHAR:
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}
