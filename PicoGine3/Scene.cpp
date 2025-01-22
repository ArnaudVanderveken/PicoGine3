// ReSharper disable CppExpressionWithoutSideEffects
#include "pch.h"
#include "Scene.h"

#include "Systems.hpp"

void Scene::Initialize()
{
	// SYSTEMS
	m_RenderSystems.emplace_back(m_Ecs.system<Components::Transform, Components::Mesh>().each(Systems::MeshRenderer));

	// MATERIALS

	// ENTITIES
	m_Ecs.entity()
		.add<Components::Transform>()
		.add<Components::Mesh>(L"Resources/Models/viking_room.obj");
}

void Scene::Start() const
{
	for (const auto& system : m_StartSystems)
		system.run();
}

void Scene::Update() const
{
	for (const auto& system : m_UpdateSystems)
		system.run();
}

void Scene::LateUpdate() const
{
	for (const auto& system : m_LateUpdateSystems)
		system.run();
}

void Scene::FixedUpdate() const
{
	for (const auto& system : m_FixedUpdateSystems)
		system.run();
}

void Scene::Render() const
{
	for (const auto& system : m_RenderSystems)
		system.run();
}
