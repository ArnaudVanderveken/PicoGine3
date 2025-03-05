#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include "Scene.h"

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

	void Update() const;
	void LateUpdate() const;
	void FixedUpdate() const;
	void Render() const;

private:
	bool m_IsInitialized{};

	std::unique_ptr<Scene> m_pScene;
};

#endif //SCENEMANAGER_H