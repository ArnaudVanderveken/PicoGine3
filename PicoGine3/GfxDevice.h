#ifndef GFXDEVICE_H
#define GFXDEVICE_H

#if defined(_DX)

class GfxDevice final
{
public:
	explicit GfxDevice() = default;
	~GfxDevice() = default;

	GfxDevice(const GfxDevice&) noexcept = delete;
	GfxDevice& operator=(const GfxDevice&) noexcept = delete;
	GfxDevice(GfxDevice&&) noexcept = delete;
	GfxDevice& operator=(GfxDevice&&) noexcept = delete;

private:


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

class GfxDevice final
{
public:
	explicit GfxDevice();
	~GfxDevice();

	GfxDevice(const GfxDevice&) noexcept = delete;
	GfxDevice& operator=(const GfxDevice&) noexcept = delete;
	GfxDevice(GfxDevice&&) noexcept = delete;
	GfxDevice& operator=(GfxDevice&&) noexcept = delete;

	[[nodiscard]] VkCommandPool GetCommandPool() const;
	[[nodiscard]] VkDevice GetDevice() const;
	[[nodiscard]] VkPhysicalDeviceProperties GetPhysicalDeviceProperties() const;
	[[nodiscard]] VkSurfaceKHR GetSurface() const;
	[[nodiscard]] VkQueue GetGraphicsQueue() const;
	[[nodiscard]] VkQueue GetPresentQueue() const;
	[[nodiscard]] QueueFamilyIndices GetQueueFamilyIndices() const;

	[[nodiscard]] SwapChainSupportDetails SwapChainSupport() const;
	[[nodiscard]] uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
	[[nodiscard]] VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
	[[nodiscard]] VkFormatProperties GetFormatProperties(VkFormat format) const;

	[[nodiscard]] VkCommandBuffer BeginSingleTimeCmdBuffer() const;
	void EndSingleTimeCmdBuffer(VkCommandBuffer commandBuffer) const;

	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const;
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
	void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const;
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const;
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED) const;

private:

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
		//VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME,
#endif //defined(_DEBUG)
	};

	std::vector<const char*> m_OptionalInstanceExtensions{};

	std::vector<const char*> m_DeviceExtensions
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	VkInstance m_VkInstance;
	VkSurfaceKHR m_VkSurface;
	VkPhysicalDevice m_VkPhysicalDevice;
	VkPhysicalDeviceProperties m_VkPhysicalDeviceProperties;
	QueueFamilyIndices m_QueueFamilyIndices;
	VkDevice m_VkDevice;
	VkQueue m_VkGraphicsQueue, m_VkPresentQueue;
	VkCommandPool m_VkCommandPool;

	void CreateVkInstance();
	void CreateSurface();
	void SelectPhysicalDevice();
	bool IsDeviceSuitable(VkPhysicalDevice device) const;
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
	void CreateLogicalDevice();
	bool CheckDeviceExtensionsSupport(VkPhysicalDevice device) const;
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) const;
	void CreateCommandPool();

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

#endif //GFXDEVICE_H