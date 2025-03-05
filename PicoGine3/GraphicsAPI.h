#ifndef GRAPHICSAPI_H
#define GRAPHICSAPI_H

#include "GfxDevice.h"

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

#include <vulkan/vulkan.h>

struct UBO
{
	alignas(16) XMFLOAT4X4 m_ModelMat;
	alignas(16) XMFLOAT4X4 m_ViewMat;
	alignas(16) XMFLOAT4X4 m_ProjectionMat;
};

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

	void CreateVertexBuffer(const void* pBufferData, VkDeviceSize bufferSize, VkBuffer& outBuffer, VkDeviceMemory& outMemory) const;
	void CreateIndexBuffer(const void* pBufferData, VkDeviceSize bufferSize, VkBuffer& outBuffer, VkDeviceMemory& outMemory) const;

	void BeginFrame();
	void EndFrame();
	void DrawMesh(uint32_t meshDataID, uint32_t materialID, const XMMATRIX& transform) const;

private:
	bool m_IsInitialized;

	static constexpr uint32_t sk_MaxFramesInFlight{ 2 };
	uint32_t m_CurrentFrame{};
	uint32_t m_CurrentFrameSwapchainImageIndex{};

	GfxDevice m_GfxDevice;
	
	
	
	VkRenderPass m_VkRenderPass;
	VkDescriptorSetLayout m_VkDescriptorSetLayout;
	VkPipelineLayout m_VkGraphicsPipelineLayout;
	VkPipeline m_VkGraphicsPipeline;
	
	
	std::vector<VkCommandBuffer> m_VkCommandBuffers;
	std::vector<VkSemaphore> m_VkImageAvailableSemaphores;
	std::vector<VkSemaphore> m_VkRenderFinishedSemaphores;
	std::vector<VkFence> m_VkInFlightFences;
	std::vector<VkBuffer> m_VkUniformBuffers;
	std::vector<VkDeviceMemory> m_VkUniformBuffersMemory;
	std::vector<void*> m_VkUniformBuffersMapped;
	VkDescriptorPool m_VkDescriptorPool;
	std::vector<VkDescriptorSet> m_VkDescriptorSets;
	uint32_t m_MipLevels;
	VkImage m_VkTestModelTextureImage;
	VkDeviceMemory m_VkTestModelTextureImageMemory;
	VkImageView m_VkTestModelTextureImageView;
	VkSampler m_VkTestModelTextureSampler;
	VkImage m_VkDepthImage;
	VkDeviceMemory m_VkDepthImageMemory;
	VkImageView m_VkDepthImageView;
	
	static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	static VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	void CreateSwapchain();
	void CreateSwapchainImageViews();
	void CreateRenderPass();
	static std::vector<char> ReadShaderFile(const std::wstring& filename);
	[[nodiscard]] VkShaderModule CreateShaderModule(const std::vector<char>& code) const;
	void CreateGraphicsPipeline();
	void CreateFrameBuffers();
	void CreateCommandBuffer();
	void CreateSyncObjects();
	void CleanupSwapchain() const;
	void RecreateSwapchain();
	
	void CreateDescriptorSetLayout();
	void CreateUniformBuffers();
	void UpdateUniformBuffer(uint32_t currentFrame) const;
	void CreateDescriptorPool();
	void CreateDescriptorSets();
	void CreateTextureImage();
	void CreateTextureImageView();
	void CreateTextureSampler();
	void CreateDepthResources();
	
	[[nodiscard]] VkFormat FindDepthFormat() const;
	void GenerateMipmaps(VkImage image, VkFormat format, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) const;
};

#endif

#endif //GRAPHICSAPI_H