#include "pch.h"
#include "CoreSystems.h"

#include "InputManager.h"
#include "Renderer.h"
#include "SceneManager.h"
#include "Settings.h"
#include "TimeManager.h"
#include "WindowManager.h"


void CoreSystems::Initialize()
{
	m_AppHinstance = GetModuleHandle(nullptr);

	Settings::Get().Initialize();
	WindowManager::Get().Initialize();
	TimeManager::Get().Initialize();
	InputManager::Get().Initialize();
	Renderer::Get().Initialize();
	SceneManager::Get().Initialize();
}

bool CoreSystems::IsInitialized()
{
	return Settings::Get().IsInitialized()
		&& WindowManager::Get().IsInitialized()
		&& TimeManager::Get().IsInitialized()
		&& InputManager::Get().IsInitialized()
		&& Renderer::Get().IsInitialized()
		&& SceneManager::Get().IsInitialized();
}

HINSTANCE CoreSystems::GetAppHinstance() const
{
    return m_AppHinstance;
}

HRESULT CoreSystems::CoreLoop() const
{
	auto& settings{ Settings::Get() };
	auto& time{ TimeManager::Get() };
	auto& input{ InputManager::Get() };
	auto& renderer{ Renderer::Get() };
	auto& sceneManager{ SceneManager::Get() };

	time.SetTargetFPS(settings.GetMaxFPS());
	const float fixedUpdateStep{ time.GetFixedTimeStep() };
	float fixedUpdateLag{};

	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));
	do
	{
		// Start frame timer and update DeltaTime
		time.Update();

		// Windows message pump
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				return static_cast<HRESULT>(msg.wParam);
		}

        // Update inputs and exec commands
		input.UpdateAndExec();

		// Scene FixedUpdate
		fixedUpdateLag += time.GetElapsedTime();
		while (fixedUpdateLag >= fixedUpdateStep)
		{
			sceneManager.FixedUpdate();
			fixedUpdateLag -= fixedUpdateStep;
		}

        // Scene Update
		sceneManager.Update();

		// Scene LateUpdate
		sceneManager.LateUpdate();

        // Render scene (only if not minimized)
		if (!m_AppMinimized)
		{
			sceneManager.Render();
			renderer.DrawFrame();
		}

		// Clear current frame keys up/down buffer in InputManager
		input.EndFrame();

		// End frame timer and sleep for frame cap
		if (settings.IsFrameCapEnabled())
			std::this_thread::sleep_for(std::chrono::duration_cast<std::chrono::milliseconds>(time.GetTimeToNextFrame()));

	} while (msg.message != WM_QUIT);

    return S_OK;
}

void CoreSystems::SetAppMinimized(bool value)
{
	m_AppMinimized = value;
}
