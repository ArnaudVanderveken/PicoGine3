#include "pch.h"
#define VOLK_IMPLEMENTATION
#include "GfxDevice.h"

#include <format>

#include "CoreSystems.h"
#include "WindowManager.h"


#if defined(_DX)

#elif defined(_VK)

GfxDevice::GfxDevice()
{
	HandleVkResult(volkInitialize());

	CreateVkInstance();
	CreateSurface();

#if defined(_DEBUG)

	SetupDebugMessenger();

#endif //defined(_DEBUG)

	SelectPhysicalDevice();
	CreateLogicalDevice();
	CreateCommandPool();
}

GfxDevice::~GfxDevice()
{
	vkDestroyCommandPool(m_VkDevice, m_VkCommandPool, nullptr);
	vkDestroyDevice(m_VkDevice, nullptr);

#if defined(_DEBUG)

	CleanupDebugMessenger();

#endif //defined(_DEBUG)

	vkDestroySurfaceKHR(m_VkInstance, m_VkSurface, nullptr);
	vkDestroyInstance(m_VkInstance, nullptr);
}

VkCommandPool GfxDevice::GetCommandPool() const
{
	return m_VkCommandPool;
}

VkDevice GfxDevice::GetDevice() const
{
	return m_VkDevice;
}

VkPhysicalDevice GfxDevice::GetPhysicalDevice() const
{
	return m_VkPhysicalDevice;
}

VkPhysicalDeviceProperties GfxDevice::GetPhysicalDeviceProperties() const
{
	return m_VkPhysicalDeviceProperties2.properties;
}

VkPhysicalDeviceProperties2 GfxDevice::GetPhysicalDeviceProperties2() const
{
	return m_VkPhysicalDeviceProperties2;
}

VkSurfaceKHR GfxDevice::GetSurface() const
{
	return m_VkSurface;
}

DeviceQueueInfo GfxDevice::GetDeviceQueueInfo() const
{
	return m_DeviceQueueInfo;
}

SwapChainSupportDetails GfxDevice::SwapChainSupport() const
{
	return QuerySwapChainSupport(m_VkPhysicalDevice);
}

uint32_t GfxDevice::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_VkPhysicalDevice, &memProperties);

	for (uint32_t i{}; i < memProperties.memoryTypeCount; ++i)
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;

	Logger::Get().LogError(L"Failed to find suitable memory type!");
	return 0;
}

VkFormat GfxDevice::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
{
	for (const auto& format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_VkPhysicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
			return format;

		if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			return format;
	}
	Logger::Get().LogError(L"No supported format in given candidates.");
	return candidates[0]; // Never executed. Quiet "not all path returns a value"
}

VkFormatProperties GfxDevice::GetFormatProperties(VkFormat format) const
{
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(m_VkPhysicalDevice, format, &formatProperties);
	return formatProperties;
}

VkResult GfxDevice::SetVkObjectName(VkObjectType objectType, uint64_t handle, const char* name) const
{
	if (!name || !*name)
		return VK_SUCCESS;

	const VkDebugUtilsObjectNameInfoEXT ni
	{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		.objectType = objectType,
		.objectHandle = handle,
		.pObjectName = name
	};
	return vkSetDebugUtilsObjectNameEXT(m_VkDevice, &ni);
}

VkCommandBuffer GfxDevice::BeginSingleTimeCmdBuffer() const
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_VkCommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_VkDevice, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void GfxDevice::EndSingleTimeCmdBuffer(VkCommandBuffer commandBuffer) const
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_DeviceQueueInfo.m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_DeviceQueueInfo.m_GraphicsQueue);

	vkFreeCommandBuffers(m_VkDevice, m_VkCommandPool, 1, &commandBuffer);
}

VkFence GfxDevice::CreateVkFence(const char* name) const
{
	constexpr VkFenceCreateInfo createInfo
	{
	  .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
	  .flags = 0,
	};

	VkFence fence{ VK_NULL_HANDLE };
	HandleVkResult(vkCreateFence(m_VkDevice, &createInfo, nullptr, &fence));
	HandleVkResult(SetVkObjectName(VK_OBJECT_TYPE_FENCE, reinterpret_cast<uint64_t>(fence), name));

	return fence;
}

VkSemaphore GfxDevice::CreateVkSemaphore(const char* name) const
{
	constexpr VkSemaphoreCreateInfo createInfo
	{
	  .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	  .flags = 0,
	};

	VkSemaphore semaphore{ VK_NULL_HANDLE };
	HandleVkResult(vkCreateSemaphore(m_VkDevice, &createInfo, nullptr, &semaphore));
	HandleVkResult(SetVkObjectName(VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<uint64_t>(semaphore), name));

	return semaphore;
}

VkSemaphore GfxDevice::CreateVkSemaphoreTimeline(uint64_t initValue, const char* name) const
{
	const VkSemaphoreTypeCreateInfo semaphoreTypeCreateInfo
	{
	  .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
	  .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
	  .initialValue = initValue,
	};

	const VkSemaphoreCreateInfo createInfo
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = &semaphoreTypeCreateInfo,
		.flags = 0,
	};

	VkSemaphore semaphore{ VK_NULL_HANDLE };
	HandleVkResult(vkCreateSemaphore(m_VkDevice, &createInfo, nullptr, &semaphore));
	HandleVkResult(SetVkObjectName(VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<uint64_t>(semaphore), name));
	return semaphore;
}

void GfxDevice::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	HandleVkResult(vkCreateBuffer(m_VkDevice, &bufferInfo, nullptr, &buffer));

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_VkDevice, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

	HandleVkResult(vkAllocateMemory(m_VkDevice, &allocInfo, nullptr, &bufferMemory));

	vkBindBufferMemory(m_VkDevice, buffer, bufferMemory, 0);
}

void GfxDevice::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const
{
	const auto cmd{ BeginSingleTimeCmdBuffer() };

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &copyRegion);

	EndSingleTimeCmdBuffer(cmd);
}

void GfxDevice::CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	HandleVkResult(vkCreateImage(m_VkDevice, &imageInfo, nullptr, &image));

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_VkDevice, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

	HandleVkResult(vkAllocateMemory(m_VkDevice, &allocInfo, nullptr, &imageMemory));

	vkBindImageMemory(m_VkDevice, image, imageMemory, 0);
}

VkImageView GfxDevice::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	HandleVkResult(vkCreateImageView(m_VkDevice, &viewInfo, nullptr, &imageView));

	return imageView;
}

void GfxDevice::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const
{
	const auto cmd{ BeginSingleTimeCmdBuffer() };

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(
		cmd,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	EndSingleTimeCmdBuffer(cmd);
}

void GfxDevice::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex) const
{
	const auto cmd{ BeginSingleTimeCmdBuffer() };

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
	barrier.dstQueueFamilyIndex = dstQueueFamilyIndex;
	barrier.image = image;
	//barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	VkPipelineStageFlags sourceStage{};
	VkPipelineStageFlags destinationStage{};

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
		Logger::Get().LogError(L"Unsupported layout transition!");

	vkCmdPipelineBarrier(cmd, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	EndSingleTimeCmdBuffer(cmd);
}

bool GfxDevice::HasExtension(const char* ext, const std::vector<VkExtensionProperties>& extensionProperties)
{
	for (const VkExtensionProperties& p : extensionProperties)
	{
		if (strcmp(ext, p.extensionName) == 0)
			return true;
	}
	return false;
}

bool GfxDevice::IsHostVisibleSingleHeapMemory(VkPhysicalDevice physicalDevice)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	if (memProperties.memoryHeapCount != 1)
		return false;

	constexpr uint32_t flag{ VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

	for (uint32_t i{}; i < memProperties.memoryTypeCount; ++i)
		if ((memProperties.memoryTypes[i].propertyFlags & flag) == flag)
			return true;

	return false;
}

void GfxDevice::GetDeviceExtensionProps(VkPhysicalDevice physicalDevice, std::vector<VkExtensionProperties>& extensionProperties, const char* validationLayer)
{
	uint32_t numExtensions{};
	vkEnumerateDeviceExtensionProperties(physicalDevice, validationLayer, &numExtensions, nullptr);
	std::vector<VkExtensionProperties> newProperties(numExtensions);
	vkEnumerateDeviceExtensionProperties(physicalDevice, validationLayer, &numExtensions, newProperties.data());
	extensionProperties.insert(extensionProperties.end(), newProperties.begin(), newProperties.end());
}

void GfxDevice::CreateVkInstance()
{
	auto& logger{ Logger::Get() };

#if defined(_DEBUG)
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : m_DefaultValidationLayers)
	{
		bool layerFound{};
		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				m_KhronosValidationVersion = layerProperties.specVersion;
				layerFound = true;
				break;
			}
		}
		if (!layerFound)
			logger.LogError(std::wstring(L"Validation layer not found: ") + StrUtils::cstr2stdwstr(layerName));
	}

#endif //defined(_DEBUG)

#if defined(_DEBUG)
	VLDDisable(); //VLD generates false positive leaks from this VK call
#endif //defined(_DEBUG)

	uint32_t extensionCount{};
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensionProperties(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProperties.data());

#if defined(_DEBUG)
	VLDEnable();
#endif //defined(_DEBUG)

#if defined(_DEBUG)
	for (const char* layer : m_DefaultValidationLayers)
	{
		uint32_t count{};
		HandleVkResult(vkEnumerateInstanceExtensionProperties(layer, &count, nullptr));
		if (count > 0)
		{
			const size_t currentSize{ extensionProperties.size() };
			extensionProperties.resize(currentSize + count);
			HandleVkResult(vkEnumerateInstanceExtensionProperties(layer, &count, extensionProperties.data() + currentSize));
		}
	}
#endif//defined(_DEBUG)

	std::vector<const char*> instanceExtensionNames
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
	};

	if (HasExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, extensionProperties))
		instanceExtensionNames.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

#if defined(_DEBUG)

	instanceExtensionNames.emplace_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);

#endif//defined(_DEBUG)

	if (HasExtension(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME, extensionProperties))
	{
		m_HasEXTSwapchainColorspace = true;
		instanceExtensionNames.emplace_back(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);
	}

#if defined(_DEBUG)

	const std::vector<VkValidationFeatureEnableEXT> validationFeaturesEnabled
	{
		VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
		VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
	};

	const VkValidationFeaturesEXT validationFeatures
	{
		.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
		.pNext = nullptr,
		.enabledValidationFeatureCount = static_cast<uint32_t>(validationFeaturesEnabled.size()),
		.pEnabledValidationFeatures = validationFeaturesEnabled.data()
	};

#if defined(VK_EXT_layer_settings) && VK_EXT_layer_settings

	constexpr VkBool32 gpuavDescriptorChecks{ VK_FALSE }; // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/8688
	constexpr VkBool32 gpuavIndirectDrawsBuffers{ VK_FALSE }; // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/8579
	constexpr VkBool32 gpuavPostProcessDescriptorIndexing{ VK_FALSE }; // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/9222

	const std::vector<VkLayerSettingEXT> settings
	{
		{m_DefaultValidationLayers[0], "gpuav_descriptor_checks", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &gpuavDescriptorChecks},
		{m_DefaultValidationLayers[0], "gpuav_indirect_draws_buffers", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &gpuavIndirectDrawsBuffers},
		{m_DefaultValidationLayers[0], "gpuav_post_process_descriptor_indexing", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &gpuavPostProcessDescriptorIndexing},
	};

	const VkLayerSettingsCreateInfoEXT layerSettingsCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT,
		.pNext = &validationFeatures,
		.settingCount = static_cast<uint32_t>(settings.size()),
		.pSettings = settings.data(),
	};

#endif // defined(VK_EXT_layer_settings) && VK_EXT_layer_settings

#endif //defined(_DEBUG)

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "PicoGine3";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "PicoGine3";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensionNames.size());
	createInfo.ppEnabledExtensionNames = instanceExtensionNames.data();

#if defined(_DEBUG)

	createInfo.enabledLayerCount = static_cast<uint32_t>(m_DefaultValidationLayers.size());
	createInfo.ppEnabledLayerNames = m_DefaultValidationLayers.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	PopulateDebugMessengerCreateInfo(debugCreateInfo);
	debugCreateInfo.pNext = &layerSettingsCreateInfo;
	createInfo.pNext = &debugCreateInfo;

#else

	createInfo.enabledLayerCount = 0;
	createInfo.pNext = nullptr;

#endif //defined(_DEBUG)

	HandleVkResult(vkCreateInstance(&createInfo, nullptr, &m_VkInstance));
	volkLoadInstance(m_VkInstance);

	logger.LogInfo(L"Enabled instance extensions:\n.", false);
	for (const auto& extension : instanceExtensionNames)
		logger.LogInfo(std::format(L"\t{}\n", StrUtils::cstr2stdwstr(extension)), false);
}

void GfxDevice::CreateSurface()
{
	VkWin32SurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = WindowManager::Get().GetHWnd();
	createInfo.hinstance = CoreSystems::Get().GetAppHinstance();

	HandleVkResult(vkCreateWin32SurfaceKHR(m_VkInstance, &createInfo, nullptr, &m_VkSurface));
}

void GfxDevice::SelectPhysicalDevice()
{
	uint32_t deviceCount{};
	vkEnumeratePhysicalDevices(m_VkInstance, &deviceCount, nullptr);

	if (deviceCount == 0)
		Logger::Get().LogError(L"Failed to find any physical device with Vulkan support!");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_VkInstance, &deviceCount, devices.data());

	for (const auto& device : devices)
	{
		if (IsDeviceSuitable(device))
		{
			m_VkPhysicalDevice = device;
			return;
		}
	}

	Logger::Get().LogError(L"Failed to find any suitable physical device.");
}

bool GfxDevice::IsDeviceSuitable(VkPhysicalDevice device) const
{
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		return false;

	const SwapChainSupportDetails swapChainSupport{ QuerySwapChainSupport(device) };
	if (swapChainSupport.m_Formats.empty() || swapChainSupport.m_PresentModes.empty())
		return false;

	return true;
}

uint32_t GfxDevice::FindQueueFamilyIndex(VkPhysicalDevice device, VkQueueFlags flags)
{
	uint32_t queueFamilyCount{};
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	auto findDedicatedQueueFamilyIndex = [&queueFamilies](VkQueueFlags require, VkQueueFlags avoid) -> uint32_t
	{
		for (uint32_t i{}; i < queueFamilies.size(); ++i)
		{
			const bool isSuitable{ (queueFamilies[i].queueFlags & require) == require };
			const bool isDedicated{ (queueFamilies[i].queueFlags & avoid) == 0 };
			if (queueFamilies[i].queueCount && isSuitable && isDedicated)
				return i;
		}
		return DeviceQueueInfo::sk_Invalid;
	};

	// dedicated queue for compute
	if (flags & VK_QUEUE_COMPUTE_BIT)
	{
		const uint32_t q{ findDedicatedQueueFamilyIndex(flags, VK_QUEUE_GRAPHICS_BIT) };
		if (q != DeviceQueueInfo::sk_Invalid)
			return q;
	}

	// dedicated queue for transfer
	if (flags & VK_QUEUE_TRANSFER_BIT)
	{
		const uint32_t q{ findDedicatedQueueFamilyIndex(flags, VK_QUEUE_GRAPHICS_BIT) };
		if (q != DeviceQueueInfo::sk_Invalid)
			return q;
	}

	// any suitable
	return findDedicatedQueueFamilyIndex(flags, 0);
}

void GfxDevice::CreateLogicalDevice()
{
	auto& logger{ Logger::Get() };

	m_UseStaging = !IsHostVisibleSingleHeapMemory(m_VkPhysicalDevice);

	std::vector<VkExtensionProperties> allDeviceExtensions;
	GetDeviceExtensionProps(m_VkPhysicalDevice, allDeviceExtensions);

#if defined(_DEBUG)

	for (const char* layer : m_DefaultValidationLayers)
		GetDeviceExtensionProps(m_VkPhysicalDevice, allDeviceExtensions, layer);

#endif //defined(_DEBUG)

	//TODO: Add Acceleration structs and ray tracing extensions properties here

	vkGetPhysicalDeviceFeatures2(m_VkPhysicalDevice, &m_VkFeatures10);
	vkGetPhysicalDeviceProperties2(m_VkPhysicalDevice, &m_VkPhysicalDeviceProperties2);

	const uint32_t apiVersion{ m_VkPhysicalDeviceProperties2.properties.apiVersion };
	const char* physicalDeviceName{ m_VkPhysicalDeviceProperties2.properties.deviceName };
	logger.LogInfo(std::format(L"Physical device name: {}\n", StrUtils::cstr2stdwstr(physicalDeviceName)), false);
	logger.LogInfo(std::format(L"API version: {}.{}.{}.{}\n", VK_API_VERSION_MAJOR(apiVersion), VK_API_VERSION_MINOR(apiVersion), VK_API_VERSION_PATCH(apiVersion), VK_API_VERSION_VARIANT(apiVersion)));
	logger.LogInfo(std::format(L"Driver info: {} {}\n", StrUtils::cstr2stdwstr(m_VkPhysicalDeviceDriverProperties.driverName), StrUtils::cstr2stdwstr(m_VkPhysicalDeviceDriverProperties.driverInfo)), false);

	logger.LogInfo(L"Vulkan physical device extensions:\n", false);
	for (const auto& extension : allDeviceExtensions)
		logger.LogInfo(std::format(L"\t{}\n", StrUtils::cstr2stdwstr(extension.extensionName)), false);

	m_DeviceQueueInfo.m_GraphicsFamily = FindQueueFamilyIndex(m_VkPhysicalDevice, VK_QUEUE_GRAPHICS_BIT);
	m_DeviceQueueInfo.m_ComputeFamily = FindQueueFamilyIndex(m_VkPhysicalDevice, VK_QUEUE_COMPUTE_BIT);

	constexpr float queuePriority{ 1.0f };
	const std::vector<VkDeviceQueueCreateInfo> queueCreateInfos
	{
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = m_DeviceQueueInfo.m_GraphicsFamily,
			.queueCount = 1,
			.pQueuePriorities = &queuePriority
		},
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = m_DeviceQueueInfo.m_ComputeFamily,
			.queueCount = 1,
			.pQueuePriorities = &queuePriority
		}
	};
	
	const uint32_t numQueues{ queueCreateInfos[0].queueFamilyIndex == queueCreateInfos[1].queueFamilyIndex ? 1u : 2u };

	std::vector<const char*> deviceExtensionNames
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	//TODO: Add extra features here (optional)

	VkPhysicalDeviceFeatures deviceFeatures10
	{
		.geometryShader = m_VkFeatures10.features.geometryShader, // enable if supported
		.tessellationShader = m_VkFeatures10.features.tessellationShader, // enable if supported
		.sampleRateShading = VK_TRUE,
		.multiDrawIndirect = VK_TRUE,
		.drawIndirectFirstInstance = VK_TRUE,
		.depthBiasClamp = VK_TRUE,
		.fillModeNonSolid = m_VkFeatures10.features.fillModeNonSolid, // enable if supported
		.samplerAnisotropy = VK_TRUE,
		.textureCompressionBC = m_VkFeatures10.features.textureCompressionBC, // enable if supported
		.vertexPipelineStoresAndAtomics = m_VkFeatures10.features.vertexPipelineStoresAndAtomics, // enable if supported
		.fragmentStoresAndAtomics = VK_TRUE,
		.shaderImageGatherExtended = VK_TRUE,
		.shaderInt64 = m_VkFeatures10.features.shaderInt64, // enable if supported
	};

	VkPhysicalDeviceVulkan11Features deviceFeatures11
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
		.pNext = nullptr,
		.storageBuffer16BitAccess = VK_TRUE,
		.samplerYcbcrConversion = m_VkFeatures11.samplerYcbcrConversion, // enable if supported
		.shaderDrawParameters = VK_TRUE,
	};

	VkPhysicalDeviceVulkan12Features deviceFeatures12
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.pNext = &deviceFeatures11,
		.drawIndirectCount = m_VkFeatures12.drawIndirectCount, // enable if supported
		.storageBuffer8BitAccess = m_VkFeatures12.storageBuffer8BitAccess, // enable if supported
		.uniformAndStorageBuffer8BitAccess = m_VkFeatures12.uniformAndStorageBuffer8BitAccess, // enable if supported
		.shaderFloat16 = m_VkFeatures12.shaderFloat16, // enable if supported
		.descriptorIndexing = VK_TRUE,
		.shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
		.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
		.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE,
		.descriptorBindingUpdateUnusedWhilePending = VK_TRUE,
		.descriptorBindingPartiallyBound = VK_TRUE,
		.descriptorBindingVariableDescriptorCount = VK_TRUE,
		.runtimeDescriptorArray = VK_TRUE,
		.scalarBlockLayout = VK_TRUE,
		.uniformBufferStandardLayout = VK_TRUE,
		.hostQueryReset = m_VkFeatures12.hostQueryReset, // enable if supported
		.timelineSemaphore = VK_TRUE,
		.bufferDeviceAddress = VK_TRUE,
		.vulkanMemoryModel = m_VkFeatures12.vulkanMemoryModel, // enable if supported
		.vulkanMemoryModelDeviceScope = m_VkFeatures12.vulkanMemoryModelDeviceScope, // enable if supported
	};

	VkPhysicalDeviceVulkan13Features deviceFeatures13
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.pNext = &deviceFeatures12,
		.subgroupSizeControl = VK_TRUE,
		.synchronization2 = VK_TRUE,
		.dynamicRendering = VK_TRUE,
		.maintenance4 = VK_TRUE,
	};

	// Check missing extensions
	std::wstring missingExtensions;
	for (const char* ext : deviceExtensionNames)
		if (!HasExtension(ext, allDeviceExtensions))
			missingExtensions += L"\t" + StrUtils::cstr2stdwstr(ext) + L"\n";

	if (!missingExtensions.empty())
		logger.LogError(std::format(L"Missing Vulkan device extensions: {}\n", missingExtensions));
	

	// Check missing features
	std::wstring missingFeatures;
#define CHECK_VULKAN_FEATURE(reqFeatures, availFeatures, feature, version)     \
	if ((reqFeatures.feature == VK_TRUE) && (availFeatures.feature == VK_FALSE)) \
		missingFeatures.append(L"\t" version L"." #feature L"\n");

#define CHECK_FEATURE_1_0(feature) CHECK_VULKAN_FEATURE(deviceFeatures10, m_VkFeatures10.features, feature, L"1.0 ");
	CHECK_FEATURE_1_0(robustBufferAccess);
	CHECK_FEATURE_1_0(fullDrawIndexUint32);
	CHECK_FEATURE_1_0(imageCubeArray);
	CHECK_FEATURE_1_0(independentBlend);
	CHECK_FEATURE_1_0(geometryShader);
	CHECK_FEATURE_1_0(tessellationShader);
	CHECK_FEATURE_1_0(sampleRateShading);
	CHECK_FEATURE_1_0(dualSrcBlend);
	CHECK_FEATURE_1_0(logicOp);
	CHECK_FEATURE_1_0(multiDrawIndirect);
	CHECK_FEATURE_1_0(drawIndirectFirstInstance);
	CHECK_FEATURE_1_0(depthClamp);
	CHECK_FEATURE_1_0(depthBiasClamp);
	CHECK_FEATURE_1_0(fillModeNonSolid);
	CHECK_FEATURE_1_0(depthBounds);
	CHECK_FEATURE_1_0(wideLines);
	CHECK_FEATURE_1_0(largePoints);
	CHECK_FEATURE_1_0(alphaToOne);
	CHECK_FEATURE_1_0(multiViewport);
	CHECK_FEATURE_1_0(samplerAnisotropy);
	CHECK_FEATURE_1_0(textureCompressionETC2);
	CHECK_FEATURE_1_0(textureCompressionASTC_LDR);
	CHECK_FEATURE_1_0(textureCompressionBC);
	CHECK_FEATURE_1_0(occlusionQueryPrecise);
	CHECK_FEATURE_1_0(pipelineStatisticsQuery);
	CHECK_FEATURE_1_0(vertexPipelineStoresAndAtomics);
	CHECK_FEATURE_1_0(fragmentStoresAndAtomics);
	CHECK_FEATURE_1_0(shaderTessellationAndGeometryPointSize);
	CHECK_FEATURE_1_0(shaderImageGatherExtended);
	CHECK_FEATURE_1_0(shaderStorageImageExtendedFormats);
	CHECK_FEATURE_1_0(shaderStorageImageMultisample);
	CHECK_FEATURE_1_0(shaderStorageImageReadWithoutFormat);
	CHECK_FEATURE_1_0(shaderStorageImageWriteWithoutFormat);
	CHECK_FEATURE_1_0(shaderUniformBufferArrayDynamicIndexing);
	CHECK_FEATURE_1_0(shaderSampledImageArrayDynamicIndexing);
	CHECK_FEATURE_1_0(shaderStorageBufferArrayDynamicIndexing);
	CHECK_FEATURE_1_0(shaderStorageImageArrayDynamicIndexing);
	CHECK_FEATURE_1_0(shaderClipDistance);
	CHECK_FEATURE_1_0(shaderCullDistance);
	CHECK_FEATURE_1_0(shaderFloat64);
	CHECK_FEATURE_1_0(shaderInt64);
	CHECK_FEATURE_1_0(shaderInt16);
	CHECK_FEATURE_1_0(shaderResourceResidency);
	CHECK_FEATURE_1_0(shaderResourceMinLod);
	CHECK_FEATURE_1_0(sparseBinding);
	CHECK_FEATURE_1_0(sparseResidencyBuffer);
	CHECK_FEATURE_1_0(sparseResidencyImage2D);
	CHECK_FEATURE_1_0(sparseResidencyImage3D);
	CHECK_FEATURE_1_0(sparseResidency2Samples);
	CHECK_FEATURE_1_0(sparseResidency4Samples);
	CHECK_FEATURE_1_0(sparseResidency8Samples);
	CHECK_FEATURE_1_0(sparseResidency16Samples);
	CHECK_FEATURE_1_0(sparseResidencyAliased);
	CHECK_FEATURE_1_0(variableMultisampleRate);
	CHECK_FEATURE_1_0(inheritedQueries);
#undef CHECK_FEATURE_1_0

#define CHECK_FEATURE_1_1(feature) CHECK_VULKAN_FEATURE(deviceFeatures11, m_VkFeatures11, feature, L"1.1 ");
	CHECK_FEATURE_1_1(storageBuffer16BitAccess);
	CHECK_FEATURE_1_1(uniformAndStorageBuffer16BitAccess);
	CHECK_FEATURE_1_1(storagePushConstant16);
	CHECK_FEATURE_1_1(storageInputOutput16);
	CHECK_FEATURE_1_1(multiview);
	CHECK_FEATURE_1_1(multiviewGeometryShader);
	CHECK_FEATURE_1_1(multiviewTessellationShader);
	CHECK_FEATURE_1_1(variablePointersStorageBuffer);
	CHECK_FEATURE_1_1(variablePointers);
	CHECK_FEATURE_1_1(protectedMemory);
	CHECK_FEATURE_1_1(samplerYcbcrConversion);
	CHECK_FEATURE_1_1(shaderDrawParameters);
#undef CHECK_FEATURE_1_1

#define CHECK_FEATURE_1_2(feature) CHECK_VULKAN_FEATURE(deviceFeatures12, m_VkFeatures12, feature, L"1.2 ");
	CHECK_FEATURE_1_2(samplerMirrorClampToEdge);
	CHECK_FEATURE_1_2(drawIndirectCount);
	CHECK_FEATURE_1_2(storageBuffer8BitAccess);
	CHECK_FEATURE_1_2(uniformAndStorageBuffer8BitAccess);
	CHECK_FEATURE_1_2(storagePushConstant8);
	CHECK_FEATURE_1_2(shaderBufferInt64Atomics);
	CHECK_FEATURE_1_2(shaderSharedInt64Atomics);
	CHECK_FEATURE_1_2(shaderFloat16);
	CHECK_FEATURE_1_2(shaderInt8);
	CHECK_FEATURE_1_2(descriptorIndexing);
	CHECK_FEATURE_1_2(shaderInputAttachmentArrayDynamicIndexing);
	CHECK_FEATURE_1_2(shaderUniformTexelBufferArrayDynamicIndexing);
	CHECK_FEATURE_1_2(shaderStorageTexelBufferArrayDynamicIndexing);
	CHECK_FEATURE_1_2(shaderUniformBufferArrayNonUniformIndexing);
	CHECK_FEATURE_1_2(shaderSampledImageArrayNonUniformIndexing);
	CHECK_FEATURE_1_2(shaderStorageBufferArrayNonUniformIndexing);
	CHECK_FEATURE_1_2(shaderStorageImageArrayNonUniformIndexing);
	CHECK_FEATURE_1_2(shaderInputAttachmentArrayNonUniformIndexing);
	CHECK_FEATURE_1_2(shaderUniformTexelBufferArrayNonUniformIndexing);
	CHECK_FEATURE_1_2(shaderStorageTexelBufferArrayNonUniformIndexing);
	CHECK_FEATURE_1_2(descriptorBindingUniformBufferUpdateAfterBind);
	CHECK_FEATURE_1_2(descriptorBindingSampledImageUpdateAfterBind);
	CHECK_FEATURE_1_2(descriptorBindingStorageImageUpdateAfterBind);
	CHECK_FEATURE_1_2(descriptorBindingStorageBufferUpdateAfterBind);
	CHECK_FEATURE_1_2(descriptorBindingUniformTexelBufferUpdateAfterBind);
	CHECK_FEATURE_1_2(descriptorBindingStorageTexelBufferUpdateAfterBind);
	CHECK_FEATURE_1_2(descriptorBindingUpdateUnusedWhilePending);
	CHECK_FEATURE_1_2(descriptorBindingPartiallyBound);
	CHECK_FEATURE_1_2(descriptorBindingVariableDescriptorCount);
	CHECK_FEATURE_1_2(runtimeDescriptorArray);
	CHECK_FEATURE_1_2(samplerFilterMinmax);
	CHECK_FEATURE_1_2(scalarBlockLayout);
	CHECK_FEATURE_1_2(imagelessFramebuffer);
	CHECK_FEATURE_1_2(uniformBufferStandardLayout);
	CHECK_FEATURE_1_2(shaderSubgroupExtendedTypes);
	CHECK_FEATURE_1_2(separateDepthStencilLayouts);
	CHECK_FEATURE_1_2(hostQueryReset);
	CHECK_FEATURE_1_2(timelineSemaphore);
	CHECK_FEATURE_1_2(bufferDeviceAddress);
	CHECK_FEATURE_1_2(bufferDeviceAddressCaptureReplay);
	CHECK_FEATURE_1_2(bufferDeviceAddressMultiDevice);
	CHECK_FEATURE_1_2(vulkanMemoryModel);
	CHECK_FEATURE_1_2(vulkanMemoryModelDeviceScope);
	CHECK_FEATURE_1_2(vulkanMemoryModelAvailabilityVisibilityChains);
	CHECK_FEATURE_1_2(shaderOutputViewportIndex);
	CHECK_FEATURE_1_2(shaderOutputLayer);
	CHECK_FEATURE_1_2(subgroupBroadcastDynamicId);
#undef CHECK_FEATURE_1_2

#define CHECK_FEATURE_1_3(feature) CHECK_VULKAN_FEATURE(deviceFeatures13, m_VkFeatures13, feature, L"1.3 ");
	CHECK_FEATURE_1_3(robustImageAccess);
	CHECK_FEATURE_1_3(inlineUniformBlock);
	CHECK_FEATURE_1_3(descriptorBindingInlineUniformBlockUpdateAfterBind);
	CHECK_FEATURE_1_3(pipelineCreationCacheControl);
	CHECK_FEATURE_1_3(privateData);
	CHECK_FEATURE_1_3(shaderDemoteToHelperInvocation);
	CHECK_FEATURE_1_3(shaderTerminateInvocation);
	CHECK_FEATURE_1_3(subgroupSizeControl);
	CHECK_FEATURE_1_3(computeFullSubgroups);
	CHECK_FEATURE_1_3(synchronization2);
	CHECK_FEATURE_1_3(textureCompressionASTC_HDR);
	CHECK_FEATURE_1_3(shaderZeroInitializeWorkgroupMemory);
	CHECK_FEATURE_1_3(dynamicRendering);
	CHECK_FEATURE_1_3(shaderIntegerDotProduct);
	CHECK_FEATURE_1_3(maintenance4);
#undef CHECK_FEATURE_1_3
#undef CHECK_VULKAN_FEATURE

	if (!missingFeatures.empty())
		logger.LogError(std::format(L"Missing Vulkan features : {}\n", missingFeatures));

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pNext = &deviceFeatures13;
	createInfo.queueCreateInfoCount = numQueues;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensionNames.size());
	createInfo.ppEnabledExtensionNames = deviceExtensionNames.data();
	createInfo.pEnabledFeatures = &deviceFeatures10;

#if defined(_DEBUG)
	createInfo.enabledLayerCount = static_cast<uint32_t>(m_DefaultValidationLayers.size());
	createInfo.ppEnabledLayerNames = m_DefaultValidationLayers.data();
#else
	createInfo.enabledLayerCount = 0;
#endif //defined(_DEBUG)

	HandleVkResult(vkCreateDevice(m_VkPhysicalDevice, &createInfo, nullptr, &m_VkDevice));
	volkLoadDevice(m_VkDevice);

	SetVkObjectName(VK_OBJECT_TYPE_DEVICE, (uint64_t)m_VkDevice, "Vulkan Device");

	vkGetDeviceQueue(m_VkDevice, m_DeviceQueueInfo.m_GraphicsFamily, 0, &m_DeviceQueueInfo.m_GraphicsQueue);
	vkGetDeviceQueue(m_VkDevice, m_DeviceQueueInfo.m_ComputeFamily, 0, &m_DeviceQueueInfo.m_ComputeQueue);
}

SwapChainSupportDetails GfxDevice::QuerySwapChainSupport(VkPhysicalDevice device) const
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_VkSurface, &details.m_Capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_VkSurface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.m_Formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_VkSurface, &formatCount, details.m_Formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_VkSurface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.m_PresentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_VkSurface, &presentModeCount, details.m_PresentModes.data());
	}

	return details;
}

void GfxDevice::CreateCommandPool()
{
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = m_DeviceQueueInfo.m_GraphicsFamily;

	HandleVkResult(vkCreateCommandPool(m_VkDevice, &poolInfo, nullptr, &m_VkCommandPool));
}

#if defined(_DEBUG)

VkBool32 VKAPI_CALL GfxDevice::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT /*messageType*/, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* /*pUserData*/)
{
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		Logger::Get().LogWarning(StrUtils::cstr2stdwstr(pCallbackData->pMessage));

	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		Logger::Get().LogError(StrUtils::cstr2stdwstr(pCallbackData->pMessage));

	return VK_FALSE;
}

void GfxDevice::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = GfxDevice::DebugCallback;
	createInfo.pUserData = nullptr; // Optional
}

VkResult GfxDevice::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	if (const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT")); func != nullptr)
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);

	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void GfxDevice::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	if (const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT")); func != nullptr)
		func(instance, debugMessenger, pAllocator);
}

void GfxDevice::SetupDebugMessenger()
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	PopulateDebugMessengerCreateInfo(createInfo);

	HandleVkResult(CreateDebugUtilsMessengerEXT(m_VkInstance, &createInfo, nullptr, &m_VkDebugMessenger));
}

void GfxDevice::CleanupDebugMessenger() const
{
	DestroyDebugUtilsMessengerEXT(m_VkInstance, m_VkDebugMessenger, nullptr);
}

#endif //#if defined(_DX) #elif defined(_VK)

#endif //defined(_DEBUG)