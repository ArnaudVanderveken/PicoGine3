#ifndef GRAPHICSAPI_H
#define GRAPHICSAPI_H

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

	void DrawTestTriangles();

private:
	bool m_IsInitialized;

};

#elif defined(_VK)

#include <vulkan/vulkan.h>
#include <optional>

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool IsComplete() const
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct Vertex
{
	XMFLOAT2 m_PositionCS;
	XMFLOAT3 m_Color;
	XMFLOAT2 m_Texcoord;

	static const VkVertexInputBindingDescription& GetBindingDescription()
	{
		static VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static const std::array<VkVertexInputAttributeDescription, 3>& GetAttributeDescriptions()
	{
		static std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, m_PositionCS);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, m_Color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, m_Texcoord);

		return attributeDescriptions;
	}
};

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

	void DrawTestTriangles();

private:
	bool m_IsInitialized;

	static inline const std::vector<Vertex> k_TestTrianglesVertices
	{
		{ { -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, {1.0f, 0.0f} },
		{ {  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, {0.0f, 0.0f} },
		{ {  0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f }, {0.0f, 1.0f} },
		{ { -0.5f,  0.5f }, { 1.0f, 1.0f, 1.0f }, {1.0f, 1.0f} }
	};

	static inline const std::vector<uint32_t> k_TestTrianglesIndices
	{
		0, 1, 2,
		2, 3, 0,
		0, 3, 2,
		2, 1, 0
	};

#if defined(_DEBUG)
	const std::vector<const char*> m_ValidationLayers
	{
		"VK_LAYER_KHRONOS_validation",
	};
#endif //defined(_DEBUG)

	const std::vector<const char*> m_RequiredInstanceExtensions
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#if defined(_DEBUG)
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif //defined(_DEBUG)
	};

	const std::vector<const char*> m_OptionalInstanceExtensions{};

	const std::vector<const char*> m_DeviceExtensions
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	static constexpr uint32_t k_MaxFramesInFlight{ 2 };
	uint32_t m_CurrentFrame{};

	VkInstance m_VkInstance;
	VkSurfaceKHR m_VkSurface;
	VkPhysicalDevice m_VkPhysicalDevice;
	VkPhysicalDeviceProperties m_VkPhysicalDeviceProperties;
	QueueFamilyIndices m_QueueFamilyIndices;
	VkDevice m_VkDevice;
	VkQueue m_VkGraphicsQueue, m_VkPresentQueue;
	VkSwapchainKHR m_VkSwapChain;
	std::vector<VkImage> m_VkSwapChainImages;
	VkFormat m_VkSwapChainImageFormat;
	VkExtent2D m_VkSwapChainExtent;
	std::vector<VkImageView> m_VkSwapChainImageViews;
	VkRenderPass m_VkRenderPass;
	VkDescriptorSetLayout m_VkDescriptorSetLayout;
	VkPipelineLayout m_VkPipelineLayout;
	VkPipeline m_VkGraphicsPipeline;
	std::vector<VkFramebuffer> m_VkFrameBuffers;
	VkCommandPool m_VkCommandPool;
	std::vector<VkCommandBuffer> m_VkCommandBuffers;
	std::vector<VkSemaphore> m_VkImageAvailableSemaphores;
	std::vector<VkSemaphore> m_VkRenderFinishedSemaphores;
	std::vector<VkFence> m_VkInFlightFences;
	VkBuffer m_VkVertexBuffer;
	VkDeviceMemory m_VkVertexBufferMemory;
	VkBuffer m_VkIndexBuffer;
	VkDeviceMemory m_VkIndexBufferMemory;
	std::vector<VkBuffer> m_VkUniformBuffers;
	std::vector<VkDeviceMemory> m_VkUniformBuffersMemory;
	std::vector<void*> m_VkUniformBuffersMapped;
	VkDescriptorPool m_VkDescriptorPool;
	std::vector<VkDescriptorSet> m_VkDescriptorSets;
	VkImage m_VkTextureImage;
	VkDeviceMemory m_VkTextureImageMemory;
	VkImageView m_VkTextureImageView;
	VkSampler m_VkTextureSampler;

	VkCommandBuffer BeginSingleTimeCmdBuffer() const;
	void EndSingleTimeCmdBuffer(VkCommandBuffer commandBuffer) const;

	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const;
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
	void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const;
	VkImageView CreateImageView(VkImage image, VkFormat format) const;
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED) const;

	void CreateVkInstance();
	void CreateSurface();
	void SelectPhysicalDevice();
	int GradeDevice(VkPhysicalDevice device) const;
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
	void CreateLogicalDevice();
	bool CheckDeviceExtensionsSupport(VkPhysicalDevice device) const;
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) const;
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
	void CreateCommandPool();
	void CreateCommandBuffer();
	void RecordTestTrianglesCmdBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) const;
	void CreateSyncObjects();
	void CleanupSwapchain() const;
	void RecreateSwapchain();
	void CreateVertexBuffer();
	void CreateIndexBuffer();
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
	void CreateDescriptorSetLayout();
	void CreateUniformBuffers();
	void UpdateUniformBuffer(uint32_t currentFrame) const;
	void CreateDescriptorPool();
	void CreateDescriptorSets();
	void CreateTextureImage();
	void CreateTextureImageView();
	void CreateTextureSampler();

#if defined(_DEBUG)
	VkDebugUtilsMessengerEXT m_VkDebugMessenger;

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
	static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	void SetupDebugMessenger();
	void CleanupDebugMessenger() const;
#endif //defined(_DEBUG)
};

#endif

#endif //GRAPHICSAPI_H