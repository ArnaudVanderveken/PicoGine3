#include "pch.h"
#include "Scene.h"

#include "Systems.hpp"

void Scene::Initialize()
{
	m_Ecs.system<>().kind(flecs::OnUpdate).each(Systems::Test);
}

void Scene::RunSystems() const
{
	if (!m_Ecs.progress())
		Logger::Get().LogError(L"ECS failed to run");
}
