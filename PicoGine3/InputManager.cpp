#include "pch.h"
#include "InputManager.h"

#include "CoreSystems.h"

bool InputManager::IsInitialized() const
{
	return m_IsInitialized;
}

void InputManager::RunCommands() const
{
	if (m_FrameKeyUp[KC_F10])
		CoreSystems::Get().GetWindowManager()->SetFullscreenState(WindowManager::WindowFullscreenState::None);

	if (m_FrameKeyUp[KC_F11])
		CoreSystems::Get().GetWindowManager()->SetFullscreenState(WindowManager::WindowFullscreenState::Fullscreen);

	if (m_FrameKeyUp[KC_F12])
		CoreSystems::Get().GetWindowManager()->SetFullscreenState(WindowManager::WindowFullscreenState::Borderless);

	/*if (m_FrameKeyUp[KC_V])
		Renderer::Get().ToggleVSync();*/

	if (m_FrameKeyUp[KC_ESCAPE])
	{
		SDL_Event quitEvent;
		quitEvent.type = SDL_QUIT;
		SDL_PushEvent(&quitEvent);
	}
}

void InputManager::EndFrame()
{
	std::ranges::fill(m_FrameKeyDown, 0);
	std::ranges::fill(m_FrameKeyUp, 0);
}

void InputManager::Flush()
{
	std::ranges::fill(m_CurrentKeys, 0);
	std::ranges::fill(m_FrameKeyDown, 0);
	std::ranges::fill(m_FrameKeyUp, 0);
}

void InputManager::KeyDown(Keycode key)
{
	if (m_CurrentKeys[key])
		return;

	m_CurrentKeys[key] = true;
	m_FrameKeyDown[key] = true;
}

void InputManager::KeyUp(Keycode key)
{
	if (!m_CurrentKeys[key])
		return;

	m_CurrentKeys[key] = false;
	m_FrameKeyUp[key] = true;
}

bool InputManager::IsKeyDown(Keycode key) const
{
	return m_FrameKeyDown[key];
}

bool InputManager::IsKeyUp(Keycode key) const
{
	return m_FrameKeyUp[key];
}

bool InputManager::IsKeyPressed(Keycode key) const
{
	return m_CurrentKeys[key];
}