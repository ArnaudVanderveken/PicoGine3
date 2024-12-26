#ifndef GRAPHICSAPI_H
#define GRAPHICSAPI_H
#include <optional>

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

private:
	bool m_IsInitialized;

};

#elif defined(_VK)

#include <vulkan/vulkan.h>

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

class GraphicsAPI
{
public:
	explicit GraphicsAPI();
	~GraphicsAPI();

	GraphicsAPI(const GraphicsAPI&) noexcept = delete;
	GraphicsAPI& operator=(const GraphicsAPI&) noexcept = delete;
	GraphicsAPI(GraphicsAPI&&) noexcept = delete;
	GraphicsAPI& operator=(GraphicsAPI&&) noexcept = delete;

	void DrawTestTriangle() const;

private:
	bool m_IsInitialized;

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

	VkInstance m_VkInstance;
	VkSurfaceKHR m_VkSurface;
	VkPhysicalDevice m_VkPhysicalDevice;
	QueueFamilyIndices m_QueueFamilyIndices;
	VkDevice m_VkDevice;
	VkQueue m_VkGraphicsQueue, m_VkPresentQueue;
	VkSwapchainKHR m_VkSwapChain;
	std::vector<VkImage> m_VkSwapChainImages;
	VkFormat m_VkSwapChainImageFormat;
	VkExtent2D m_VkSwapChainExtent;
	std::vector<VkImageView> m_VkSwapChainImageViews;
	VkRenderPass m_VkRenderPass;
	VkPipelineLayout m_VkPipelineLayout;
	VkPipeline m_VkGraphicsPipeline;
	std::vector<VkFramebuffer> m_VkFrameBuffers;
	VkCommandPool m_VkCommandPool;
	VkCommandBuffer m_VkCommandBuffer;
	VkSemaphore m_VkImageAvailableSemaphore;
	VkSemaphore m_VkRenderFinishedSemaphore;
	VkFence m_VkInFlightFence;

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
	VkShaderModule CreateShaderModule(const std::vector<char>& code) const;
	void CreateGraphicsPipeline();
	void CreateFrameBuffers();
	void CreateCommandPool();
	void CreateCommandBuffer();
	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) const;
	void CreateSyncObjects();

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