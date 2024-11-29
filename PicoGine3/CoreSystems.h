#ifndef CORESYSTEMS_H
#define CORESYSTEMS_H

#include "WindowManager.h"

class CoreSystems final : public Singleton<CoreSystems>
{
	friend class Singleton<CoreSystems>;

	explicit CoreSystems();

public:
	~CoreSystems() override = default;
	CoreSystems(const CoreSystems&) noexcept = delete;
	CoreSystems& operator=(const CoreSystems&) noexcept = delete;
	CoreSystems(CoreSystems&&) noexcept = delete;
	CoreSystems& operator=(CoreSystems&&) noexcept = delete;

	[[nodiscard]] bool IsInitialized() const;

	[[nodiscard]] WindowManager* GetWindowManager() const;

	void CoreLoop();

private:
	std::unique_ptr<WindowManager> m_pWindowManager;

};

#endif //CORESYSTEMS_H