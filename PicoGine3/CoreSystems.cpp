#include "pch.h"
#include "CoreSystems.h"

#include "InputManager.h"
#include "Renderer.h"
#include "Settings.h"
#include "WindowManager.h"


CoreSystems::CoreSystems() :
	m_AppHinstance{ GetModuleHandle(nullptr) },
	m_AppMinimized{}
{
}

bool CoreSystems::IsInitialized()
{
	return Settings::Get().IsInitialized()
		&& WindowManager::Get().IsInitialized()
		&& InputManager::Get().IsInitialized()
		&& Renderer::Get().IsInitialized();
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

        // Render scene (only if not minimized)
		if (!m_AppMinimized)
			Renderer::Get().DrawFrame();

		// Clear current frame keys up/down buffer in InputManager
		InputManager::Get().EndFrame();

	} while (msg.message != WM_QUIT);

    return S_OK;
}

void CoreSystems::SetAppMinimized(bool value)
{
	m_AppMinimized = value;
}
