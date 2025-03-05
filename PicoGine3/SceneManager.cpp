#include "pch.h"
#include "SceneManager.h"

void SceneManager::Initialize()
{
	m_pScene = std::make_unique<Scene>();
	m_pScene->Initialize();
	m_pScene->Start();

	m_IsInitialized = true;
}

bool SceneManager::IsInitialized() const
{
	return m_IsInitialized;
}

void SceneManager::Update() const
{
	m_pScene->Update();
}

void SceneManager::LateUpdate() const
{
	m_pScene->LateUpdate();
}

void SceneManager::FixedUpdate() const
{
	m_pScene->FixedUpdate();
}

void SceneManager::Render() const
{
	m_pScene->Render();
}
