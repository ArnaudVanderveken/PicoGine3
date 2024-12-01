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

	void UpdateAndExec() const;
	void EndFrame();
	void Flush();

	void KeyDown(Keycode key);
	void KeyUp(Keycode key);

	[[nodiscard]] bool IsKeyDown(Keycode key) const;
	[[nodiscard]] bool IsKeyUp(Keycode key) const;
	[[nodiscard]] bool IsKeyPressed(Keycode key) const;

	/*[[nodiscard]] const XMINT2& GetMousePosition() const;
	[[nodiscard]] const XMINT2& GetMouseDelta() const;*/

private:
	bool m_IsInitialized;

	std::array<bool, 256> m_CurrentKeys{};
	std::array<bool, 256> m_FrameKeyDown{};
	std::array<bool, 256> m_FrameKeyUp{};

	/*XMINT2 m_MousePosition;
	XMINT2 m_PreviousMousePosition;
	XMINT2 m_DeltaMousePosition;*/

};

#endif //INPUTMANAGER_H