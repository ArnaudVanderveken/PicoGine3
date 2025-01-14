#include "pch.h"
#include "Renderer.h"


void Renderer::Initialize()
{
	m_pGraphicsAPI = std::make_unique<GraphicsAPI>();
	m_IsInitialized = true;
}

bool Renderer::IsInitialized() const
{
	return m_IsInitialized;
}

void Renderer::DrawFrame() const
{
	if (m_pGraphicsAPI->IsInitialized())
		m_pGraphicsAPI->DrawTestModel();
	else
		Logger::Get().LogWarning(L"GraphicsAPI is uninitialized. Ignoring call");
}
