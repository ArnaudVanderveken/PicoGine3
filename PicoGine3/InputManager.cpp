#include "pch.h"
#include "InputManager.h"

#include "WindowManager.h"

InputManager::InputManager() :
	m_IsInitialized{ true }/*,
	m_MousePosition{ 0, 0 },
	m_PreviousMousePosition{ 0, 0 },
	m_DeltaMousePosition{ 0, 0 }*/
{
}

bool InputManager::IsInitialized() const
{
	return m_IsInitialized;
}

void InputManager::UpdateAndExec() const
{
	// Exec commands
	if (m_FrameKeyUp[KC_F10])
		WindowManager::Get().SetFullscreenState(WindowFullscreenState::None);

	if (m_FrameKeyUp[KC_F11])
		/*WindowManager::Get().SetFullscreenState(WindowFullscreenState::Fullscreen);

	if (m_FrameKeyUp[KC_F12])*/
		WindowManager::Get().SetFullscreenState(WindowFullscreenState::Borderless);

	/*if (m_FrameKeyUp[KC_V])
		Renderer::Get().ToggleVSync();*/

	if (m_FrameKeyUp[KC_ESCAPE])
		::PostQuitMessage(0);
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
	if (!m_IsInitialized || m_CurrentKeys[key])
		return;

	m_CurrentKeys[key] = true;
	m_FrameKeyDown[key] = true;
}

void InputManager::KeyUp(Keycode key)
{
	if (!m_IsInitialized || !m_CurrentKeys[key])
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

//const XMINT2& InputManager::GetMousePosition() const
//{
//	return m_MousePosition;
//}
//
//const XMINT2& InputManager::GetMouseDelta() const
//{
//	return m_DeltaMousePosition;
//}
