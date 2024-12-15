#ifndef RENDERER_H
#define RENDERER_H

#include "Singleton.h"

#include "GraphicsAPI.h"


class Renderer final : public Singleton<Renderer>
{
	friend class Singleton<Renderer>;
public:
	explicit Renderer();
	~Renderer() override = default;

	Renderer(const Renderer&) noexcept = delete;
	Renderer& operator=(const Renderer&) noexcept = delete;
	Renderer(Renderer&&) noexcept = delete;
	Renderer& operator=(Renderer&&) noexcept = delete;

	[[nodiscard]] bool IsInitialized() const;

private:
	bool m_IsInitialized;

	std::unique_ptr<GraphicsAPI> m_pGraphicsAPI;

};

#endif //RENDERER_H