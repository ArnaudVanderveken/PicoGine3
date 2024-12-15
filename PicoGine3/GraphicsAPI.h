#ifndef GRAPHICSAPI_H
#define GRAPHICSAPI_H

#if defined(_DX12)
#include <d3d12.h>
#include <dxgi1_6.h>
#elif defined(_VK)
#include <vulkan/vulkan.h>
#endif


class GraphicsAPI
{
public:
	explicit GraphicsAPI();
	~GraphicsAPI();

	GraphicsAPI(const GraphicsAPI&) noexcept = delete;
	GraphicsAPI& operator=(const GraphicsAPI&) noexcept = delete;
	GraphicsAPI(GraphicsAPI&&) noexcept = delete;
	GraphicsAPI& operator=(GraphicsAPI&&) noexcept = delete;

private:
	

};

#endif //GRAPHICSAPI_H