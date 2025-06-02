#include "pch.h"
#include "Renderer.h"


void Renderer::Initialize()
{
	m_pGraphicsAPI = std::make_unique<GraphicsAPI>();
	m_IsInitialized = m_pGraphicsAPI->IsInitialized();
}

bool Renderer::IsInitialized() const
{
	return m_IsInitialized;
}

GraphicsAPI* Renderer::GetGraphicsAPI() const
{
	return m_pGraphicsAPI.get();
}

void Renderer::DrawMesh(uint32_t meshDataID, uint32_t materialID, const XMFLOAT4X4& transform)
{
	m_RenderEntries.emplace_back(RenderEntry{ meshDataID, materialID, transform });
}

void Renderer::DrawFrame()
{
	// For now, brute force rendering, no batching no instancing, simply rendering all models in order.
	m_pGraphicsAPI->BeginFrame();

	for (const auto& entry : m_RenderEntries)
		m_pGraphicsAPI->DrawMesh(entry.m_MeshDataID, entry.m_MaterialID, entry.m_TransformMatrix);

	m_pGraphicsAPI->EndFrame();

	m_RenderEntries.clear();
}
