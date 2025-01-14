#ifndef CORESYSTEMS_H
#define CORESYSTEMS_H


class CoreSystems final : public Singleton<CoreSystems>
{
	friend class Singleton<CoreSystems>;
	explicit CoreSystems() = default;

public:
	~CoreSystems() override = default;
	CoreSystems(const CoreSystems&) noexcept = delete;
	CoreSystems& operator=(const CoreSystems&) noexcept = delete;
	CoreSystems(CoreSystems&&) noexcept = delete;
	CoreSystems& operator=(CoreSystems&&) noexcept = delete;

	void Initialize();
	[[nodiscard]] static bool IsInitialized();

	[[nodiscard]] HINSTANCE GetAppHinstance() const;

	HRESULT CoreLoop() const;
	void SetAppMinimized(bool value);

private:
	HINSTANCE m_AppHinstance{};
	bool m_AppMinimized{};
};

#endif //CORESYSTEMS_H