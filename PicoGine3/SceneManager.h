#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H


class SceneManager final : public Singleton<SceneManager>
{
	friend class Singleton<SceneManager>;
	explicit SceneManager() = default;

public:
	~SceneManager() override = default;

	SceneManager(const SceneManager&) noexcept = delete;
	SceneManager& operator=(const SceneManager&) noexcept = delete;
	SceneManager(SceneManager&&) noexcept = delete;
	SceneManager& operator=(SceneManager&&) noexcept = delete;

	void Initialize();
	[[nodiscard]] bool IsInitialized() const;

private:
	bool m_IsInitialized{};


};

#endif //SCENEMANAGER_H