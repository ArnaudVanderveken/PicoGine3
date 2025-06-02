// ReSharper disable CppExpressionWithoutSideEffects
#include "pch.h"
#include "Scene.h"


void Scene::Initialize()
{
	using namespace Systems;
	using namespace Components;

	Renderer::Get().GetGraphicsAPI()->AcquireCommandBuffer();

	// SYSTEMS
	m_RenderSystems.emplace_back(MeshRenderer);

	// MATERIALS

	// ENTITIES
	auto entity = m_Ecs.create();
	m_Ecs.emplace<Transform>(entity, XMFLOAT3{ -1.0f, 0.0f, 0.0f }, XMFLOAT3{ 0.0f, 0.0f, 0.0f }, XMFLOAT3{ 1.0f, 1.0f, 1.0f });
	m_Ecs.emplace<Mesh>(entity, L"Resources/Models/viking_room.obj");

	entity = m_Ecs.create();
	m_Ecs.emplace<Transform>(entity, XMFLOAT3{ 2.0f, 0.0f, 0.0f }, XMFLOAT3{ 0.0f, 0.0f, 0.0f }, XMFLOAT3{ 1.0f, 1.0f, 1.0f });
	m_Ecs.emplace<Mesh>(entity, L"Resources/Models/viking_room.obj");

	Renderer::Get().GetGraphicsAPI()->SubmitCommandBuffer();
}

void Scene::Start()
{
	for (const auto& system : m_StartSystems)
		system(m_Ecs);
}

void Scene::Update()
{
	for (const auto& system : m_UpdateSystems)
		system(m_Ecs);
}

void Scene::LateUpdate()
{
	for (const auto& system : m_LateUpdateSystems)
		system(m_Ecs);
}

void Scene::FixedUpdate()
{
	for (const auto& system : m_FixedUpdateSystems)
		system(m_Ecs);
}

void Scene::Render()
{
	for (const auto& system : m_RenderSystems)
		system(m_Ecs);
}
