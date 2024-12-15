#include "pch.h"
#include "Renderer.h"

Renderer::Renderer() :
	m_IsInitialized{ false },
	m_pGraphicsAPI{ std::make_unique<GraphicsAPI>() }
{
	m_IsInitialized = true;
}

bool Renderer::IsInitialized() const
{
	return m_IsInitialized;
}
