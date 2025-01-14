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
	void RunSystems() const;

private:
	flecs::world m_Ecs{};

};

#endif //SCENE_H