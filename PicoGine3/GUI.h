#ifndef GUI_H
#define GUI_H

#include "Singleton.h"

class GUI final : public Singleton<GUI>
{
	friend class Singleton<GUI>;
	explicit GUI();

public:
	~GUI() override;

	GUI(const GUI&) noexcept = delete;
	GUI& operator=(const GUI&) noexcept = delete;
	GUI(GUI&&) noexcept = delete;
	GUI& operator=(GUI&&) noexcept = delete;

	void Initialize();
	[[nodiscard]] bool IsInitialized() const;

	void BeginFrame();
	void EndFrame();

private:
	bool m_IsInitialized{};

};

#endif //GUI_H