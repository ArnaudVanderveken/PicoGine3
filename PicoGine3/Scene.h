#ifndef SCENE_H
#define SCENE_H

#pragma warning(push)
#pragma warning(disable:6386)
#pragma warning(disable:4127)
#pragma warning(disable:4244)
#include "flecs.h"
#pragma warning(pop)

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
	void Start() const;
	void Update() const;
	void LateUpdate() const;
	void FixedUpdate() const;
	void Render() const;

private:
	flecs::world m_Ecs{};

	std::vector<flecs::system> m_StartSystems{};
	std::vector<flecs::system> m_UpdateSystems{};
	std::vector<flecs::system> m_LateUpdateSystems{};
	std::vector<flecs::system> m_FixedUpdateSystems{};
	std::vector<flecs::system> m_RenderSystems{};
};

#endif //SCENE_H