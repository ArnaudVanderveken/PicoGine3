#include "pch.h"
#include "GfxSwapchain.h"

#include <format>

#include "GraphicsAPI.h"
#include "Settings.h"
#include "WindowManager.h"

#if defined(_DX)

#elif defined(_VK)

GfxSwapchain::GfxSwapchain(GraphicsAPI* pGraphicsAPI) :
	m_pGraphicsAPI{ pGraphicsAPI }
{
	CreateSwapchain();
	CreateDepthResources();
	CreateRenderPass();
	CreateFrameBuffers();
	CreateSyncObjects();
}

GfxSwapchain::~GfxSwapchain()
{
	const auto& device{ m_pGraphicsAPI->GetGfxDevice()->GetDevice() };

	for (size_t i{}; i < sk_MaxFramesInFlight; ++i)
		vkDestroySemaphore(device, m_AcquireSemaphores[i], nullptr);

	vkDestroyRenderPass(device, m_VkRenderPass, nullptr);

	CleanupSwapchain();
	vkDestroySwapchainKHR(device, m_VkSwapChain, nullptr);
}

VkFramebuffer GfxSwapchain::GetCurrentFrameBuffer() const
{
	return m_VkFrameBuffers[m_CurrentFrameSwapchainImageIndex];
}

VkRenderPass GfxSwapchain::GetRenderPass() const
{
	return m_VkRenderPass;
}

VkImageView GfxSwapchain::GetImageView(int index) const
{
	assert(index >= 0 && index < m_VkSwapChainImageViews.size() && L"Invalid swap chain image view index.");
	return m_VkSwapChainImageViews[index];
}

size_t GfxSwapchain::GetImageCount() const
{
	return m_VkSwapChainImages.size();
}

VkFormat GfxSwapchain::GetSwapChainImageFormat() const
{
	return m_VkSwapChainColorFormat;
}

VkExtent2D GfxSwapchain::GetSwapChainExtent() const
{
	return m_VkSwapChainExtent;
}

uint32_t GfxSwapchain::Width() const
{
	return m_VkSwapChainExtent.width;
}

uint32_t GfxSwapchain::Height() const
{
	return m_VkSwapChainExtent.height;
}

float GfxSwapchain::AspectRatio() const
{
	return static_cast<float>(m_VkSwapChainExtent.width) / static_cast<float>(m_VkSwapChainExtent.height);
}

uint64_t GfxSwapchain::GetCurrentFrameIndex() const
{
	return m_CurrentFrame;
}

VkFormat GfxSwapchain::FindDepthFormat() const
{
	return m_pGraphicsAPI->GetGfxDevice()->FindSupportedFormat({ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void GfxSwapchain::SetCurrentFrameTimelineWaitValue(uint64_t value)
{
	m_TimelineWaitValues[m_CurrentFrameSwapchainImageIndex] = value;
}

VkResult GfxSwapchain::Present(VkSemaphore semaphore)
{
	const auto& deviceQueueInfo{ m_pGraphicsAPI->GetGfxDevice()->GetDeviceQueueInfo() };

	const VkPresentInfoKHR presentInfo
	{
	  .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
	  .waitSemaphoreCount = 1,
	  .pWaitSemaphores = &semaphore,
	  .swapchainCount = 1u,
	  .pSwapchains = &m_VkSwapChain,
	  .pImageIndices = &m_CurrentFrameSwapchainImageIndex,
	};
	const VkResult result = vkQueuePresentKHR(deviceQueueInfo.m_GraphicsQueue, &presentInfo);

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR && result != VK_ERROR_OUT_OF_DATE_KHR)
		HandleVkResult(result);

	m_GetNextImage = true;
	++m_CurrentFrame;

	return result;
}

void GfxSwapchain::RecreateSwapchain()
{
	const auto& device{ m_pGraphicsAPI->GetGfxDevice()->GetDevice() };
	vkDeviceWaitIdle(device);

	CleanupSwapchain();

	CreateSwapchain();
	CreateDepthResources();
	CreateFrameBuffers();
}

VkResult GfxSwapchain::AcquireImage()
{
	const auto& device{ m_pGraphicsAPI->GetGfxDevice()->GetDevice() };

	if (m_GetNextImage)
	{
		const auto timelineSemaphore{ m_pGraphicsAPI->GetTimelineSemaphore() };

		const VkSemaphoreWaitInfo waitInfo
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
			.semaphoreCount = 1,
			.pSemaphores = &timelineSemaphore,
			.pValues = &m_TimelineWaitValues[m_CurrentFrameSwapchainImageIndex],
		};

		HandleVkResult(vkWaitSemaphores(device, &waitInfo, UINT64_MAX));

		const VkSemaphore acquireSemaphore{ m_AcquireSemaphores[m_CurrentFrameSwapchainImageIndex] };

		const VkResult result{ vkAcquireNextImageKHR(device, m_VkSwapChain, UINT64_MAX, acquireSemaphore, VK_NULL_HANDLE, &m_CurrentFrameSwapchainImageIndex) };
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR && result != VK_ERROR_OUT_OF_DATE_KHR)
			HandleVkResult(result);

		m_GetNextImage = false;
		m_pGraphicsAPI->GetGfxImmediateCommands()->WaitSemaphore(acquireSemaphore);
	}

	// TODO: Transition image layout for presentation

	return VK_SUCCESS;
}

VkSurfaceFormatKHR GfxSwapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	// Select RGBA or BGRA based on first available format
	const auto targetFormat{ availableFormats[0].format == VK_FORMAT_B8G8R8A8_SRGB || availableFormats[0].format == VK_FORMAT_B8G8R8A8_UNORM || availableFormats[0].format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 ? VK_FORMAT_B8G8R8A8_SRGB : VK_FORMAT_R8G8B8A8_SRGB };

	for (const auto& availableFormat : availableFormats)
		if (availableFormat.format == targetFormat && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return availableFormat;

	return availableFormats[0];
}

VkPresentModeKHR GfxSwapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	const auto& settings{ Settings::Get() };

	if (!settings.IsVSyncEnabled())
		for (const auto& availablePresentMode : availablePresentModes)
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				return availablePresentMode;

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D GfxSwapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		return capabilities.currentExtent;

	int width, height;
	WindowManager::Get().GetWindowDimensions(width, height);

	VkExtent2D actualExtent
	{
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height)
	};

	actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	return actualExtent;
}

void GfxSwapchain::CreateSwapchain()
{
	const auto& gfxDevice{ m_pGraphicsAPI->GetGfxDevice() };
	const auto& device{ gfxDevice->GetDevice() };
	const auto& physicalDevice{ gfxDevice->GetPhysicalDevice() };
	const auto& deviceQueueInfo{ gfxDevice->GetDeviceQueueInfo() };
	const auto& surface{ gfxDevice->GetSurface() };

	VkBool32 queueFamilySupportsPresentation{};
	HandleVkResult(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, deviceQueueInfo.m_GraphicsFamily, surface, &queueFamilySupportsPresentation));
	if (!queueFamilySupportsPresentation)
		Logger::Get().LogError(L"The queue family used with the swapchain does not support presentation.");

	const SwapChainSupportDetails swapChainSupport{ gfxDevice->SwapChainSupport()};

	const VkSurfaceFormatKHR surfaceFormat{ ChooseSwapSurfaceFormat(swapChainSupport.m_Formats) };
	const VkPresentModeKHR presentMode{ ChooseSwapPresentMode(swapChainSupport.m_PresentModes) };
	const VkExtent2D extent{ ChooseSwapExtent(swapChainSupport.m_Capabilities) };

	uint32_t imageCount{ swapChainSupport.m_Capabilities.minImageCount + 1 };

	if (swapChainSupport.m_Capabilities.maxImageCount > 0 && imageCount > swapChainSupport.m_Capabilities.maxImageCount)
		imageCount = swapChainSupport.m_Capabilities.maxImageCount;

	VkImageUsageFlags usageFlags{ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT };

	VkSurfaceCapabilitiesKHR surfaceCapabilities{};
	HandleVkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities));

	VkFormatProperties formatProperties{};
	vkGetPhysicalDeviceFormatProperties(physicalDevice, surfaceFormat.format, &formatProperties);

	const bool isStorageSupported{ (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT) > 0 };
	const bool isTilingOptimalSupported{ (formatProperties.optimalTilingFeatures & VK_IMAGE_USAGE_STORAGE_BIT) > 0 };

	if (isStorageSupported && isTilingOptimalSupported)
		usageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;

	const bool isCompositeAlphaOpaqueSupported{ (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) != 0 };

	const VkSwapchainKHR oldSwapchain{ m_VkSwapChain };

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = usageFlags;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = 1;
	createInfo.pQueueFamilyIndices = &deviceQueueInfo.m_GraphicsFamily;
	createInfo.preTransform = swapChainSupport.m_Capabilities.currentTransform;
	createInfo.compositeAlpha = isCompositeAlphaOpaqueSupported ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR : VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = oldSwapchain;

	HandleVkResult(vkCreateSwapchainKHR(device, &createInfo, nullptr, &m_VkSwapChain));

	if (oldSwapchain != VK_NULL_HANDLE)
		vkDestroySwapchainKHR(device, oldSwapchain, nullptr);

	std::vector<VkImage> swapchainImages;
	HandleVkResult(vkGetSwapchainImagesKHR(device, m_VkSwapChain, &imageCount, nullptr));
	swapchainImages.resize(imageCount);
	HandleVkResult(vkGetSwapchainImagesKHR(device, m_VkSwapChain, &imageCount, swapchainImages.data()));

	m_VkSwapChainColorFormat = surfaceFormat.format;
	m_VkSwapChainExtent = extent;

	char debugNameImage[256]{};
	char debugNameImageView[256]{};

	for (uint32_t i{}; i < imageCount; ++i)
	{
		snprintf(debugNameImage, sizeof(debugNameImage) - 1, "Image: swapchain %u", i);
		snprintf(debugNameImageView, sizeof(debugNameImageView) - 1, "Image View: swapchain %u", i);
		
		GfxImage image{ m_pGraphicsAPI };
		image.m_VkImage = swapchainImages[i];
		image.m_VkUsageFlags = usageFlags;
		image.m_VkExtent = VkExtent3D{ .width = m_VkSwapChainExtent.width, .height = m_VkSwapChainExtent.height, .depth = 1 };
		image.m_VkType = VK_IMAGE_TYPE_2D;
		image.m_VkImageFormat = m_VkSwapChainColorFormat;
		image.m_IsSwapchainImage = true;
		image.m_IsOwningVkImage = false;
		image.m_IsDepthFormat = GfxImage::IsDepthFormat(m_VkSwapChainColorFormat);
		image.m_IsStencilFormat = GfxImage::IsStencilFormat(m_VkSwapChainColorFormat);

		HandleVkResult(gfxDevice->SetVkObjectName(VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64_t>(image.m_VkImage), debugNameImage));

		image.m_ImageView = image.CreateImageView(gfxDevice,
												  VK_IMAGE_VIEW_TYPE_2D,
												  m_VkSwapChainColorFormat,
												  VK_IMAGE_ASPECT_COLOR_BIT,
												  0,
												  VK_REMAINING_MIP_LEVELS,
												  0,
												  1,
												  {},
												  nullptr,
												  debugNameImageView);
	}
}

void GfxSwapchain::CreateDepthResources()
{
	const VkFormat depthFormat{ FindDepthFormat() };
	m_DepthImages.reserve(m_SwapChainImages.size());

	char debugNameImageView[256]{};

	for (size_t i{}; i < m_SwapChainImages.size(); ++i)
	{
		snprintf(debugNameImageView, sizeof(debugNameImageView) - 1, "Image View: swapchain depth %u", i);

		m_DepthImages.emplace_back(GfxImage{ m_pGraphicsAPI });
		GfxImage& image{ m_DepthImages[i] };
		m_pGraphicsAPI->GetGfxDevice()->CreateImage(m_VkSwapChainExtent.width, m_VkSwapChainExtent.height, 1, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image.m_VkImage, image.m_VkMemory[0]);
		image.m_ImageView = image.CreateImageView(m_pGraphicsAPI->GetGfxDevice(),
												  VK_IMAGE_VIEW_TYPE_2D,
												  depthFormat,
												  VK_IMAGE_ASPECT_DEPTH_BIT,
												  0,
												  1,
												  0,
												  1,
												  {},
												  nullptr,
												  debugNameImageView);
	}
}

void GfxSwapchain::CreateFrameBuffers()
{
	const auto& device{ m_pGfxDevice->GetDevice() };

	m_VkFrameBuffers.resize(m_VkSwapChainImageViews.size());

	for (size_t i{}; i < m_VkSwapChainImageViews.size(); ++i) {
		const std::array<VkImageView, 2> attachments
		{
			m_VkSwapChainImageViews[i],
			m_VkDepthImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_VkRenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = m_VkSwapChainExtent.width;
		framebufferInfo.height = m_VkSwapChainExtent.height;
		framebufferInfo.layers = 1;

		HandleVkResult(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_VkFrameBuffers[i]));
	}
}

void GfxSwapchain::CleanupSwapchain()
{
	const auto& device{ m_pGfxDevice->GetDevice() };

	for (const auto framebuffer : m_VkFrameBuffers)
		vkDestroyFramebuffer(device, framebuffer, nullptr);

	m_DepthImages.clear();
	m_SwapChainImages.clear();
}

void GfxSwapchain::CreateRenderPass()
{
	const auto& device{ m_pGfxDevice->GetDevice() };

	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_VkSwapChainColorFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = FindDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	const std::array<VkAttachmentDescription, 2> attachments{ colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	HandleVkResult(vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_VkRenderPass));
}

void GfxSwapchain::CreateSyncObjects()
{
	for (size_t i{}; i < sk_MaxFramesInFlight; ++i)
		m_AcquireSemaphores[i] = m_pGfxDevice->CreateVkSemaphore(std::format("GfxSwapchain::m_AcquireSemaphore[%i]", i).c_str());
}

#endif