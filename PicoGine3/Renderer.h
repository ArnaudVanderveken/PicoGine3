#ifndef RENDERER_H
#define RENDERER_H

#include "Singleton.h"

#include "GraphicsAPI.h"


class Renderer final : public Singleton<Renderer>
{
	friend class Singleton<Renderer>;
	explicit Renderer() = default;

	struct RenderEntry
	{
		uint32_t m_MeshDataID{};
		uint32_t m_MaterialID{};
		XMFLOAT4X4 m_TransformMatrix{};
	};

public:
	~Renderer() override = default;

	Renderer(const Renderer&) noexcept = delete;
	Renderer& operator=(const Renderer&) noexcept = delete;
	Renderer(Renderer&&) noexcept = delete;
	Renderer& operator=(Renderer&&) noexcept = delete;

	void Initialize();
	[[nodiscard]] bool IsInitialized() const;

	[[nodiscard]] GraphicsAPI* GetGraphicsAPI() const;

	void DrawMesh(uint32_t meshDataID, uint32_t materialID, const XMFLOAT4X4& transform);
	void DrawFrame() const;

private:
	bool m_IsInitialized{};

	std::unique_ptr<GraphicsAPI> m_pGraphicsAPI{};

	std::vector<RenderEntry> m_RenderEntries{};
};

#endif //RENDERER_H