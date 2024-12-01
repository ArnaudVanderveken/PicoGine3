#include "pch.h"
#include "CoreSystems.h"

#include "InputManager.h"
#include "Settings.h"
#include "WindowManager.h"


CoreSystems::CoreSystems() :
	m_AppHinstance{ GetModuleHandle(nullptr) }
{
}

LRESULT CoreSystems::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static RECT clientRect{}; //Moved outside of switch to suppress warning

	switch (message)
	{
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;

	case WM_SIZE:
		::GetClientRect(hWnd, &clientRect);

		WindowManager::Get().UpdateWindowRect(clientRect);
		break;
	
	case WM_LBUTTONDOWN:
		InputManager::Get().KeyDown(KC_MOUSE1);
		break;

	case WM_LBUTTONUP:
		InputManager::Get().KeyUp(KC_MOUSE1);
		break;

	case WM_RBUTTONDOWN:
		InputManager::Get().KeyDown(KC_MOUSE2);
		break;

	case WM_RBUTTONUP:
		InputManager::Get().KeyUp(KC_MOUSE2);
		break;

	case WM_MBUTTONDOWN:
		InputManager::Get().KeyDown(KC_MOUSE3);
		break;

	case WM_MBUTTONUP:
		InputManager::Get().KeyUp(KC_MOUSE3);
		break;

	case WM_XBUTTONDOWN:
		InputManager::Get().KeyDown(wParam >> 16 == 1 ? KC_MOUSE4 : KC_MOUSE5);
		break;

	case WM_XBUTTONUP:
		InputManager::Get().KeyUp(wParam >> 16 == 1 ? KC_MOUSE4 : KC_MOUSE5);
		break;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		InputManager::Get().KeyDown(static_cast<Keycode>(wParam));
		break;

	case WM_KEYUP:
	case WM_SYSKEYUP:
		InputManager::Get().KeyUp(static_cast<Keycode>(wParam));
		break;

	case WM_SYSCHAR:
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

bool CoreSystems::IsInitialized()
{
	return Settings::Get().IsInitialized()
		&& WindowManager::Get().IsInitialized()
		&& InputManager::Get().IsInitialized();
}

HINSTANCE CoreSystems::GetAppHinstance() const
{
    return m_AppHinstance;
}

HRESULT CoreSystems::CoreLoop() const
{
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));
	do
	{
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				return static_cast<HRESULT>(msg.wParam);
		}

        // Update inputs and exec commands
		InputManager::Get().UpdateAndExec();

        // Update scene

        // Render scene

		// Clear current frame keys up/down buffer in InputManager
		InputManager::Get().EndFrame();

	} while (msg.message != WM_QUIT);

    return S_OK;
}
