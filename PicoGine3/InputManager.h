#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include "Keycodes.h"

class InputManager final : public Singleton<InputManager>
{
	friend class Singleton<InputManager>;
	explicit InputManager();

public:
	~InputManager() override = default;

	InputManager(const InputManager&) noexcept = delete;
	InputManager& operator=(const InputManager&) noexcept = delete;
	InputManager(InputManager&&) noexcept = delete;
	InputManager& operator=(InputManager&&) noexcept = delete;

	[[nodiscard]] bool IsInitialized() const;

	void UpdateAndExec();
	void EndFrame();
	void Flush();

	void ProcessRawInput(HRAWINPUT hRawInput);

	[[nodiscard]] bool IsKeyDown(KeyCode key) const;
	[[nodiscard]] bool IsKeyUp(KeyCode key) const;
	[[nodiscard]] bool IsKeyPressed(KeyCode key) const;

	[[nodiscard]] bool IsControllerButtonDown(ControllerCode key) const;
	[[nodiscard]] bool IsControllerButtonUp(ControllerCode key) const;
	[[nodiscard]] bool IsControllerButtonPressed(ControllerCode key) const;
	[[nodiscard]] SHORT GetControllerLeftStickX() const;
	[[nodiscard]] SHORT GetControllerLeftStickY() const;
	[[nodiscard]] SHORT GetControllerRightStickX() const;
	[[nodiscard]] SHORT GetControllerRightStickY() const;
	[[nodiscard]] BYTE GetControllerLeftTrigger() const;
	[[nodiscard]] BYTE GetControllerRightTrigger() const;

	[[nodiscard]] const XMINT2& GetMousePosition() const;
	[[nodiscard]] const XMINT2& GetMouseDelta() const;

private:
	bool m_IsInitialized;

	std::array<RAWINPUTDEVICE, 2> m_RawDevices;

	XMINT2 m_MousePosition;
	XMINT2 m_MouseDelta;

	std::array<bool, 256> m_CurrentKeys;
	std::array<bool, 256> m_FrameKeyDown;
	std::array<bool, 256> m_FrameKeyUp;

	bool m_ControllerConnected;
	XINPUT_STATE m_ControllerState;
	std::array<bool, GP_COUNT> m_ControllerButtonsDown;
	std::array<bool, GP_COUNT> m_ControllerButtonsUp;

	void UpdateControllerState();
};

#endif //INPUTMANAGER_H