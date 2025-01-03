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

	void DrawTestModel();

private:
	bool m_IsInitialized;

};

#elif defined(_VK)

#include <vulkan/vulkan.h>
#include <optional>

struct QueueFamilyIndices
{
	std::optional<uint32_t> m_GraphicsFamily{};
	std::optional<uint32_t> m_PresentFamily{};

	[[nodiscard]] bool IsComplete() const
	{
		return m_GraphicsFamily.has_value() && m_PresentFamily.has_value();
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR m_Capabilities{};
	std::vector<VkSurfaceFormatKHR> m_Formats{};
	std::vector<VkPresentModeKHR> m_PresentModes{};
};

struct Vertex
{
	XMFLOAT3 m_Position{ 0.0f, 0.0f, 0.0f };
	XMFLOAT3 m_Color{ 1.0f, 1.0f, 1.0f };
	XMFLOAT2 m_Texcoord{ 0.0f, 0.0f };

	bool operator==(const Vertex& other) const
	{
		return m_Position.x == other.m_Position.x
			&& m_Position.y == other.m_Position.y
			&& m_Position.z == other.m_Position.z
			&& m_Color.x == other.m_Color.x
			&& m_Color.y == other.m_Color.y
			&& m_Color.z == other.m_Color.z
			&& m_Texcoord.x == other.m_Texcoord.x
			&& m_Texcoord.y == other.m_Texcoord.y;
	}

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
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, m_Position);

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

template <>
struct std::hash<Vertex>
{
	std::size_t operator()(const Vertex& vertex) const noexcept
	{
		const std::size_t h1 = std::hash<float>()(vertex.m_Position.x) ^ (std::hash<float>()(vertex.m_Position.y) << 1) ^ (std::hash<float>()(vertex.m_Position.z) << 2);
		const std::size_t h2 = std::hash<float>()(vertex.m_Color.x) ^ (std::hash<float>()(vertex.m_Color.y) << 1) ^ (std::hash<float>()(vertex.m_Color.z) << 2);
		const std::size_t h3 = std::hash<float>()(vertex.m_Texcoord.x) ^ (std::hash<float>()(vertex.m_Texcoord.y) << 1);
		return h1 ^ h2 ^ h3;
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

	void DrawTestModel();

private:
	bool m_IsInitialized;

#if defined(_DEBUG)
	std::vector<const char*> m_ValidationLayers
	{
		"VK_LAYER_KHRONOS_validation",
	};
#endif //defined(_DEBUG)

	std::vector<const char*> m_RequiredInstanceExtensions
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#if defined(_DEBUG)
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif //defined(_DEBUG)
	};

	std::vector<const char*> m_OptionalInstanceExtensions{};

	std::vector<const char*> m_DeviceExtensions
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	static constexpr uint32_t sk_MaxFramesInFlight{ 2 };
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
	VkPipelineLayout m_VkGraphicsPipelineLayout;
	VkPipeline m_VkGraphicsPipeline;
	std::vector<VkFramebuffer> m_VkFrameBuffers;
	VkCommandPool m_VkCommandPool;
	std::vector<VkCommandBuffer> m_VkCommandBuffers;
	std::vector<VkSemaphore> m_VkImageAvailableSemaphores;
	std::vector<VkSemaphore> m_VkRenderFinishedSemaphores;
	std::vector<VkFence> m_VkInFlightFences;
	std::vector<Vertex> m_TestModelVertices;
	std::vector<uint32_t> m_TestModelIndices;
	VkBuffer m_VkTestModelVertexBuffer;
	VkDeviceMemory m_VkTestModelVertexBufferMemory;
	VkBuffer m_VkTestModelIndexBuffer;
	VkDeviceMemory m_VkTestModelIndexBufferMemory;
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

	[[nodiscard]] VkCommandBuffer BeginSingleTimeCmdBuffer() const;
	void EndSingleTimeCmdBuffer(VkCommandBuffer commandBuffer) const;

	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const;
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
	void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const;
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const;
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED) const;

	void CreateVkInstance();
	void CreateSurface();
	void SelectPhysicalDevice();
	uint32_t GradeDevice(VkPhysicalDevice device) const;
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
	[[nodiscard]] uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
	void CreateDescriptorSetLayout();
	void CreateUniformBuffers();
	void UpdateUniformBuffer(uint32_t currentFrame) const;
	void CreateDescriptorPool();
	void CreateDescriptorSets();
	void CreateTextureImage();
	void CreateTextureImageView();
	void CreateTextureSampler();
	void CreateDepthResources();
	[[nodiscard]] VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
	[[nodiscard]] VkFormat FindDepthFormat() const;
	static bool HasStencilComponent(VkFormat format);
	void LoadTestModel();
	void GenerateMipmaps(VkImage image, VkFormat format, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) const;

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