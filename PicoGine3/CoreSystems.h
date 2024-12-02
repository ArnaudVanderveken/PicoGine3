#ifndef CORESYSTEMS_H
#define CORESYSTEMS_H


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

	[[nodiscard]] static bool IsInitialized();

	[[nodiscard]] HINSTANCE GetAppHinstance() const;

	HRESULT CoreLoop() const;

private:

	HINSTANCE m_AppHinstance;
};

#endif //CORESYSTEMS_H