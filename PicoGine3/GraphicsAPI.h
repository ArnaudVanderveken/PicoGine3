#ifndef GRAPHICSAPI_H
#define GRAPHICSAPI_H

#include "GfxBuffer.h"
#include "GfxDevice.h"
#include "GfxImmediateCommands.h"
#include "GfxSwapchain.h"
#include "ShaderModulePool.h"

#if defined(_DX12)

#include <d3d12.h>
#include <dxgi1_6.h>

class GraphicsAPI
{
public:
	explicit GraphicsAPI();
	~GraphicsAPI();

	GraphicsAPI(const GraphicsAPI&) noexcept = delete;
	GraphicsAPI& operator=(const GraphicsAPI&) noexcept = delete;
	GraphicsAPI(GraphicsAPI&&) noexcept = delete;
	GraphicsAPI& operator=(GraphicsAPI&&) noexcept = delete;

	[[nodiscard]] bool IsInitialized() const;

	void BeginFrame();
	void EndFrame();
	void DrawMesh(uint32_t meshDataID, uint32_t materialID, const XMMATRIX& transform) const;

private:
	bool m_IsInitialized;

};

#elif defined(_VK)

#include <volk.h>

struct PerFrameUBO
{
	alignas(16) XMFLOAT4X4 m_ViewMat;
	alignas(16) XMFLOAT4X4 m_ProjMat;
	alignas(16) XMFLOAT4X4 m_ViewInvMat;
	alignas(16) XMFLOAT4X4 m_ProjInvMat;
	alignas(16) XMFLOAT4X4 m_ViewProjMat;
	alignas(16) XMFLOAT4X4 m_ViewProjInvMat;
};

struct PushConstants
{
	alignas(16) XMFLOAT4X4 m_ModelMat;
};

class GraphicsAPI final
{
public:
	explicit GraphicsAPI();
	~GraphicsAPI();

	GraphicsAPI(const GraphicsAPI&) noexcept = delete;
	GraphicsAPI& operator=(const GraphicsAPI&) noexcept = delete;
	GraphicsAPI(GraphicsAPI&&) noexcept = delete;
	GraphicsAPI& operator=(GraphicsAPI&&) noexcept = delete;

	[[nodiscard]] bool IsInitialized() const;
	[[nodiscard]] GfxDevice* GetGfxDevice() const;

	void ReleaseBuffer(const VkBuffer& buffer, const VkDeviceMemory& memory) const;

	void BeginFrame() const;
	void EndFrame() const;
	void DrawMesh(uint32_t meshDataID, uint32_t materialID, const XMFLOAT4X4& transform) const;

private:
	bool m_IsInitialized;

	std::unique_ptr<GfxDevice> m_pGfxDevice;
	std::unique_ptr<GfxSwapchain> m_pGfxSwapchain;
	std::unique_ptr<GfxImmediateCommands> m_pGfxImmediateCommands;
	VkSemaphore m_TimelineSemaphore;
	std::unique_ptr<ShaderModulePool> m_pShaderModulePool;
	
	VkDescriptorSetLayout m_VkDescriptorSetLayout;
	VkPipelineLayout m_VkGraphicsPipelineLayout;
	VkPipeline m_VkGraphicsPipeline;
	
	std::vector<VkCommandBuffer> m_VkCommandBuffers;

	std::vector<std::unique_ptr<GfxBuffer>> m_PerFrameUBO;
	VkDescriptorPool m_VkDescriptorPool;
	std::vector<VkDescriptorSet> m_VkDescriptorSets;
	uint32_t m_MipLevels;
	VkImage m_VkTestModelTextureImage;
	VkDeviceMemory m_VkTestModelTextureImageMemory;
	VkImageView m_VkTestModelTextureImageView;
	VkSampler m_VkTestModelTextureSampler;

	void CreateGraphicsPipeline();
	
	void CreateCommandBuffer();	
	void CreateDescriptorSetLayout();
	void CreateUniformBuffers();
	void UpdatePerFrameUBO() const;
	void CreateDescriptorPool();
	void CreateDescriptorSets();
	void CreateTextureImage();
	void CreateTextureImageView();
	void CreateTextureSampler();
	
	void GenerateMipmaps(VkImage image, VkFormat format, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) const;
};

#endif

#endif //GRAPHICSAPI_H