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

//#include <vulkan/vulkan.h>
#include <volk.h>

struct DeviceQueueInfo
{
	static constexpr uint32_t sk_Invalid{ 0xffffffff };
	uint32_t m_GraphicsFamily{ sk_Invalid };
	uint32_t m_ComputeFamily{ sk_Invalid };
	//uint32_t m_PresentFamily{ sk_Invalid };
	VkQueue m_GraphicsQueue{ VK_NULL_HANDLE };
	VkQueue m_ComputeQueue{ VK_NULL_HANDLE };
	//VkQueue m_PresentQueue{ VK_NULL_HANDLE };

	[[nodiscard]] bool IsComplete() const	
	{
		return m_GraphicsFamily != sk_Invalid && m_ComputeFamily != sk_Invalid /*&& m_PresentFamily != sk_Invalid*/;
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR m_Capabilities{};
	std::vector<VkSurfaceFormatKHR> m_Formats{};
	std::vector<VkPresentModeKHR> m_PresentModes{};
};

class GraphicsAPI;

class GfxDevice final
{
public:
	explicit GfxDevice();
	~GfxDevice();

	GfxDevice(const GfxDevice&) noexcept = delete;
	GfxDevice& operator=(const GfxDevice&) noexcept = delete;
	GfxDevice(GfxDevice&&) noexcept = delete;
	GfxDevice& operator=(GfxDevice&&) noexcept = delete;

	[[nodiscard]] VkDevice GetDevice() const;
	[[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const;
	[[nodiscard]] const VkPhysicalDeviceProperties& GetPhysicalDeviceProperties() const;
	[[nodiscard]] const VkPhysicalDeviceProperties2& GetPhysicalDeviceProperties2() const;
	[[nodiscard]] const VkPhysicalDeviceLimits& GetPhysicalDeviceLimits() const;
	[[nodiscard]] VkSurfaceKHR GetSurface() const;
	[[nodiscard]] DeviceQueueInfo GetDeviceQueueInfo() const;

	[[nodiscard]] SwapChainSupportDetails SwapChainSupport() const;
	[[nodiscard]] uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
	[[nodiscard]] VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
	[[nodiscard]] VkFormatProperties GetFormatProperties(VkFormat format) const;

	VkResult SetVkObjectName(VkObjectType objectType, uint64_t handle, const char* name) const;

	[[nodiscard]] VkFence CreateVkFence(const char* name = nullptr) const;
	[[nodiscard]] VkSemaphore CreateVkSemaphore(const char* name = nullptr) const;
	[[nodiscard]] VkSemaphore CreateVkSemaphoreTimeline(uint64_t initValue, const char* name = nullptr) const;

	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const;
	void CopyBuffer(VkCommandBuffer cmdBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
	void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const;
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const;
	void CopyBufferToImage(VkCommandBuffer cmdBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;

private:

#if defined(_DEBUG)
	std::vector<const char*> m_DefaultValidationLayers
	{
		"VK_LAYER_KHRONOS_validation",
	};
#endif //defined(_DEBUG)

	VkInstance m_VkInstance;

	VkSurfaceKHR m_VkSurface;

	VkPhysicalDevice m_VkPhysicalDevice;
	VkPhysicalDeviceDepthStencilResolveProperties m_VkPhysicalDeviceDepthStencilResolveProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES, nullptr };
	VkPhysicalDeviceDriverProperties m_VkPhysicalDeviceDriverProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES, &m_VkPhysicalDeviceDepthStencilResolveProperties };
	VkPhysicalDeviceVulkan12Properties m_VkPhysicalDeviceVulkan12Properties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES, &m_VkPhysicalDeviceDriverProperties };
	VkPhysicalDeviceProperties2 m_VkPhysicalDeviceProperties2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, &m_VkPhysicalDeviceVulkan12Properties, VkPhysicalDeviceProperties{} };
	VkPhysicalDeviceVulkan13Features m_VkFeatures13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	VkPhysicalDeviceVulkan12Features m_VkFeatures12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, .pNext = &m_VkFeatures13 };
	VkPhysicalDeviceVulkan11Features m_VkFeatures11{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES, .pNext = &m_VkFeatures12 };
	VkPhysicalDeviceFeatures2 m_VkFeatures10{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &m_VkFeatures11 };

	DeviceQueueInfo m_DeviceQueueInfo;
	VkDevice m_VkDevice;

	uint32_t m_KhronosValidationVersion{};
	bool m_HasEXTSwapchainColorspace{ false };
	bool m_UseStaging{ false };

	static bool HasExtension(const char* ext, const std::vector<VkExtensionProperties>& extensionProperties);
	static bool IsHostVisibleSingleHeapMemory(VkPhysicalDevice physicalDevice);
	static void GetDeviceExtensionProps(VkPhysicalDevice physicalDevice, std::vector<VkExtensionProperties>& extensionProperties, const char* validationLayer = nullptr);

	void CreateVkInstance();
	void CreateSurface();
	void SelectPhysicalDevice();
	bool IsDeviceSuitable(VkPhysicalDevice device) const;
	static uint32_t FindQueueFamilyIndex(VkPhysicalDevice device, VkQueueFlags flags);
	void CreateLogicalDevice();
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) const;

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