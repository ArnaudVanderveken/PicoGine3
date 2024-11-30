#pragma once

#include "Keycodes.h"

class InputManager final
{
public:
	explicit InputManager() = default;
	~InputManager() = default;

	InputManager(const InputManager&) noexcept = delete;
	InputManager& operator=(const InputManager&) noexcept = delete;
	InputManager(InputManager&&) noexcept = delete;
	InputManager& operator=(InputManager&&) noexcept = delete;

	[[nodiscard]] bool IsInitialized() const;

	void RunCommands() const;
	void EndFrame();
	void Flush();

	void KeyDown(Keycode key);
	void KeyUp(Keycode key);

	[[nodiscard]] bool IsKeyDown(Keycode key) const;
	[[nodiscard]] bool IsKeyUp(Keycode key) const;
	[[nodiscard]] bool IsKeyPressed(Keycode key) const;

private:
	bool m_IsInitialized;

	std::array<bool, 256> m_CurrentKeys{};
	std::array<bool, 256> m_FrameKeyDown{};
	std::array<bool, 256> m_FrameKeyUp{};

};

