#ifndef SCENE_H
#define SCENE_H

#include "Systems.hpp"

class Scene final
{
public:
	explicit Scene() = default;
	~Scene() = default;

	Scene(const Scene&) noexcept = delete;
	Scene& operator=(const Scene&) noexcept = delete;
	Scene(Scene&&) noexcept = delete;
	Scene& operator=(Scene&&) noexcept = delete;

	void Initialize();
	void Start();
	void Update();
	void LateUpdate();
	void FixedUpdate();
	void Render();

private:
	entt::registry m_Ecs{};

	std::vector<Systems::system_type> m_StartSystems{};
	std::vector<Systems::system_type> m_UpdateSystems{};
	std::vector<Systems::system_type> m_LateUpdateSystems{};
	std::vector<Systems::system_type> m_FixedUpdateSystems{};
	std::vector<Systems::system_type> m_RenderSystems{};
};

#endif //SCENE_H