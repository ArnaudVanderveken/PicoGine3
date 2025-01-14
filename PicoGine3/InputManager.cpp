#include "pch.h"
#include "InputManager.h"

#include <hidusage.h>

#include "Settings.h"
#include "WindowManager.h"


void InputManager::Initialize()
{
	const auto& hwnd = WindowManager::Get().GetHWnd();

	m_RawDevices[0] = { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_KEYBOARD, RIDEV_INPUTSINK, hwnd };
	m_RawDevices[1] = { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MOUSE, RIDEV_INPUTSINK, hwnd };

	if (!RegisterRawInputDevices(m_RawDevices.data(), static_cast<UINT>(m_RawDevices.size()), sizeof(RAWINPUTDEVICE)))
	{
		std::cout << "Failed to register raw input devices\n";
		return;
	}

	m_IsInitialized = true;
}

bool InputManager::IsInitialized() const
{
	return m_IsInitialized;
}

void InputManager::UpdateAndExec()
{
	UpdateControllerState();

	XMStoreSInt2(&m_MousePosition, XMLoadSInt2(&m_MousePosition) + XMLoadSInt2(&m_MouseDelta));

	// Exec commands
	if (IsKeyUp(KC_F10) || IsControllerButtonUp(GP_B))
		WindowManager::Get().SetFullscreenState(WindowFullscreenState::None);

	if (IsKeyUp(KC_F11) || IsControllerButtonUp(GP_A))
		WindowManager::Get().SetFullscreenState(WindowFullscreenState::Borderless);

	if (IsKeyUp(KC_V))
		Settings::Get().SetVSync(!Settings::Get().GetVSync());

	if (IsKeyUp(KC_ESCAPE))
		::PostQuitMessage(0);
}

void InputManager::EndFrame()
{
	m_MouseDelta = XMINT2{ 0, 0 };

	std::ranges::fill(m_FrameKeyDown, 0);
	std::ranges::fill(m_FrameKeyUp, 0);
}

void InputManager::Flush()
{
	m_MouseDelta = XMINT2{ 0, 0 };

	std::ranges::fill(m_CurrentKeys, 0);
	std::ranges::fill(m_FrameKeyDown, 0);
	std::ranges::fill(m_FrameKeyUp, 0);

	std::ranges::fill(m_ControllerButtonsDown, 0);
	std::ranges::fill(m_ControllerButtonsUp, 0);
}

void InputManager::ProcessRawInput(HRAWINPUT hRawInput)
{
	RAWINPUT rawInput;
	UINT dataSize = sizeof(rawInput);

	if (GetRawInputData(hRawInput, RID_INPUT, &rawInput, &dataSize, sizeof(RAWINPUTHEADER)) == -1)
		return;

	if (rawInput.header.dwType == RIM_TYPEKEYBOARD)
	{
		// Process keyboard input
		const RAWKEYBOARD& keyboard = rawInput.data.keyboard;
		const bool isKeyDown = !(keyboard.Flags & RI_KEY_BREAK);

		if (const UINT virtualKey = keyboard.VKey; virtualKey < 256)
		{
			m_FrameKeyDown[virtualKey] = isKeyDown && !m_CurrentKeys[virtualKey];
			m_FrameKeyUp[virtualKey] = !isKeyDown && m_CurrentKeys[virtualKey];
			m_CurrentKeys[virtualKey] = isKeyDown;
		}
	}
	else if (rawInput.header.dwType == RIM_TYPEMOUSE)
	{
		// Process mouse input
		const RAWMOUSE& mouse = rawInput.data.mouse;

		const float scale = static_cast<float>(GetDpiForWindow(WindowManager::Get().GetHWnd())) / 96.0f;

		m_MouseDelta.x += static_cast<int>(static_cast<float>(mouse.lLastX) * scale);
		m_MouseDelta.y += static_cast<int>(static_cast<float>(mouse.lLastY) * scale);

		if (mouse.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN)
		{
			m_FrameKeyUp[KC_MOUSE1] = !m_CurrentKeys[KC_MOUSE1];
			m_CurrentKeys[KC_MOUSE1] = true;
		}
		else if (mouse.usButtonFlags & RI_MOUSE_BUTTON_1_UP)
		{
			m_FrameKeyDown[KC_MOUSE1] = m_CurrentKeys[KC_MOUSE1];
			m_CurrentKeys[KC_MOUSE1] = false;
		}

		if (mouse.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN)
		{
			m_FrameKeyUp[KC_MOUSE2] = !m_CurrentKeys[KC_MOUSE2];
			m_CurrentKeys[KC_MOUSE2] = true;
		}
		else if (mouse.usButtonFlags & RI_MOUSE_BUTTON_2_UP)
		{
			m_FrameKeyUp[KC_MOUSE2] = m_CurrentKeys[KC_MOUSE2];
			m_CurrentKeys[KC_MOUSE2] = false;
		}

		if (mouse.usButtonFlags & RI_MOUSE_BUTTON_3_DOWN)
		{
			m_FrameKeyUp[KC_MOUSE3] = !m_CurrentKeys[KC_MOUSE3];
			m_CurrentKeys[KC_MOUSE3] = true;
		}
		else if (mouse.usButtonFlags & RI_MOUSE_BUTTON_3_UP)
		{
			m_FrameKeyUp[KC_MOUSE3] = m_CurrentKeys[KC_MOUSE3];
			m_CurrentKeys[KC_MOUSE3] = false;
		}

		if (mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN)
		{
			m_FrameKeyUp[KC_MOUSE4] = !m_CurrentKeys[KC_MOUSE4];
			m_CurrentKeys[KC_MOUSE4] = true;
		}
		else if (mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP)
		{
			m_FrameKeyUp[KC_MOUSE4] = m_CurrentKeys[KC_MOUSE4];
			m_CurrentKeys[KC_MOUSE4] = false;
		}

		if (mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN)
		{
			m_FrameKeyUp[KC_MOUSE5] = !m_CurrentKeys[KC_MOUSE5];
			m_CurrentKeys[KC_MOUSE5] = true;
		}
		else if (mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP)
		{
			m_FrameKeyUp[KC_MOUSE5] = m_CurrentKeys[KC_MOUSE5];
			m_CurrentKeys[KC_MOUSE5] = false;
		}
	}
}

bool InputManager::IsKeyDown(KeyCode key) const
{
	return m_FrameKeyDown[key];
}

bool InputManager::IsKeyUp(KeyCode key) const
{
	return m_FrameKeyUp[key];
}

bool InputManager::IsKeyPressed(KeyCode key) const
{
	return m_CurrentKeys[key];
}

bool InputManager::IsControllerButtonDown(ControllerCode key) const
{
	if (key >= GP_COUNT)
		return false;

	return m_ControllerButtonsDown[key];
}

bool InputManager::IsControllerButtonUp(ControllerCode key) const
{
	if (key >= GP_COUNT)
		return false;

	return m_ControllerButtonsUp[key];
}

bool InputManager::IsControllerButtonPressed(ControllerCode key) const
{
	if (key >= GP_COUNT)
		return false;

	return m_ControllerState.Gamepad.wButtons & 1 << key;
}

SHORT InputManager::GetControllerLeftStickX() const
{
	return abs(m_ControllerState.Gamepad.sThumbLX) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ? m_ControllerState.Gamepad.sThumbLX : static_cast<SHORT>(0);
}

SHORT InputManager::GetControllerLeftStickY() const
{
	return abs(m_ControllerState.Gamepad.sThumbLY) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ? m_ControllerState.Gamepad.sThumbLY : static_cast<SHORT>(0);
}

SHORT InputManager::GetControllerRightStickX() const
{
	return abs(m_ControllerState.Gamepad.sThumbRX) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ? m_ControllerState.Gamepad.sThumbRX : static_cast<SHORT>(0);
}

SHORT InputManager::GetControllerRightStickY() const
{
	return abs(m_ControllerState.Gamepad.sThumbRY) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ? m_ControllerState.Gamepad.sThumbRY : static_cast<SHORT>(0);
}

BYTE InputManager::GetControllerLeftTrigger() const
{
	return m_ControllerState.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ? m_ControllerState.Gamepad.bLeftTrigger : static_cast<BYTE>(0);
}

BYTE InputManager::GetControllerRightTrigger() const
{
	return m_ControllerState.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ? m_ControllerState.Gamepad.bRightTrigger : static_cast<BYTE>(0);
}

const XMINT2& InputManager::GetMousePosition() const
{
	return m_MousePosition;
}

const XMINT2& InputManager::GetMouseDelta() const
{
	return m_MouseDelta;
}

void InputManager::UpdateControllerState()
{
	XINPUT_STATE xState;

	if (XInputGetState(0, &xState) == ERROR_SUCCESS)
	{
		m_ControllerConnected = true;

		for (int i{}; i < GP_COUNT; ++i)
		{
			const bool isPressed = xState.Gamepad.wButtons & 1 << i;
			m_ControllerButtonsDown[i] = isPressed && !(m_ControllerState.Gamepad.wButtons & 1 << i);
			m_ControllerButtonsUp[i] = !isPressed && (m_ControllerState.Gamepad.wButtons & 1 << i);
		}

		m_ControllerState = xState;
	}
	else
	{
		m_ControllerConnected = false;
	}
}
