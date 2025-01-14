#include "pch.h"
#include "SceneManager.h"

void SceneManager::Initialize()
{
	m_IsInitialized = true;
}

bool SceneManager::IsInitialized() const
{
	return m_IsInitialized;
}
