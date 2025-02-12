#include "pch.h"
#include "GraphicsAPI.h"

#include "ResourceManager.h"
#include "WindowManager.h"


#if defined(_DX12)

GraphicsAPI::GraphicsAPI() :
	m_IsInitialized{ false }
{
	//m_IsInitialized = true;
}

GraphicsAPI::~GraphicsAPI()
{
}

bool GraphicsAPI::IsInitialized() const
{
	return m_IsInitialized;
}

void GraphicsAPI::DrawTestModel()
{
	
}

#elif defined(_VK)

#pragma warning(push)
#pragma warning(disable:6011)
#pragma warning(disable:26819)
#pragma warning(disable:6262)
#pragma warning(disable:6308)
#pragma warning(disable:28182)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h> //TEMPORARY
#pragma warning(pop)


GraphicsAPI::GraphicsAPI() :
	m_IsInitialized{ false },
	m_GfxDevice{}
{
	CreateSwapchain();
	CreateSwapchainImageViews();
	CreateRenderPass();
	CreateDescriptorSetLayout();
	CreateGraphicsPipeline();
	CreateDepthResources();
	CreateFrameBuffers();
	CreateTextureImage();
	CreateTextureImageView();
	CreateTextureSampler();
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
	CreateCommandBuffer();
	CreateSyncObjects();

	m_IsInitialized = true;
}

GraphicsAPI::~GraphicsAPI()
{
	const auto& device{ m_GfxDevice.GetDevice() };
	vkDeviceWaitIdle(device);

	for (size_t i{}; i < sk_MaxFramesInFlight; ++i)
	{
		vkDestroySemaphore(device, m_VkImageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(device, m_VkRenderFinishedSemaphores[i], nullptr);
		vkDestroyFence(device, m_VkInFlightFences[i], nullptr);
	}

	CleanupSwapchain();

	vkDestroySampler(device, m_VkTestModelTextureSampler, nullptr);
	vkDestroyImageView(device, m_VkTestModelTextureImageView, nullptr);
	vkDestroyImage(device, m_VkTestModelTextureImage, nullptr);
	vkFreeMemory(device, m_VkTestModelTextureImageMemory, nullptr);

	for (size_t i{}; i < sk_MaxFramesInFlight; ++i)
	{
		vkDestroyBuffer(device, m_VkUniformBuffers[i], nullptr);
		vkFreeMemory(device, m_VkUniformBuffersMemory[i], nullptr);
	}

	vkDestroyDescriptorPool(device, m_VkDescriptorPool, nullptr);

	vkDestroyDescriptorSetLayout(device, m_VkDescriptorSetLayout, nullptr);

	/*vkDestroyBuffer(device, m_VkTestModelIndexBuffer, nullptr);
	vkFreeMemory(device, m_VkTestModelIndexBufferMemory, nullptr);

	vkDestroyBuffer(device, m_VkTestModelVertexBuffer, nullptr);
	vkFreeMemory(device, m_VkTestModelVertexBufferMemory, nullptr);*/
	
	vkDestroyPipeline(device, m_VkGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, m_VkGraphicsPipelineLayout, nullptr);
	vkDestroyRenderPass(device, m_VkRenderPass, nullptr);
}

bool GraphicsAPI::IsInitialized() const
{
	return m_IsInitialized;
}

void GraphicsAPI::CreateVertexBuffer(const void* pBufferData, VkDeviceSize bufferSize, VkBuffer& outBuffer, VkDeviceMemory& outMemory) const
{
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(m_VkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, pBufferData, bufferSize);
	vkUnmapMemory(m_VkDevice, stagingBufferMemory);

	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, outBuffer, outMemory);

	CopyBuffer(stagingBuffer, outBuffer, bufferSize);

	vkDestroyBuffer(m_VkDevice, stagingBuffer, nullptr);
	vkFreeMemory(m_VkDevice, stagingBufferMemory, nullptr);
}

void GraphicsAPI::CreateIndexBuffer(const void* pBufferData, VkDeviceSize bufferSize, VkBuffer& outBuffer, VkDeviceMemory& outMemory) const
{
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(m_VkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, pBufferData, bufferSize);
	vkUnmapMemory(m_VkDevice, stagingBufferMemory);

	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, outBuffer, outMemory);

	CopyBuffer(stagingBuffer, outBuffer, bufferSize);

	vkDestroyBuffer(m_VkDevice, stagingBuffer, nullptr);
	vkFreeMemory(m_VkDevice, stagingBufferMemory, nullptr);
}

void GraphicsAPI::BeginFrame()
{
	vkWaitForFences(m_VkDevice, 1, &m_VkInFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

	const auto vkResult{ vkAcquireNextImageKHR(m_VkDevice, m_VkSwapChain, UINT64_MAX, m_VkImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &m_CurrentFrameSwapchainImageIndex) };

	if (vkResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapchain();
		return;
	}

	if (vkResult != VK_SUCCESS && vkResult != VK_SUBOPTIMAL_KHR)
		HandleVkResult(vkResult);

	vkResetFences(m_VkDevice, 1, &m_VkInFlightFences[m_CurrentFrame]);
	vkResetCommandBuffer(m_VkCommandBuffers[m_CurrentFrame], 0);

	UpdateUniformBuffer(m_CurrentFrame);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	vkBeginCommandBuffer(m_VkCommandBuffers[m_CurrentFrame], &beginInfo);

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_VkRenderPass;
	renderPassInfo.framebuffer = m_VkFrameBuffers[m_CurrentFrameSwapchainImageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_VkSwapChainExtent;
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(m_VkCommandBuffers[m_CurrentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(m_VkCommandBuffers[m_CurrentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_VkGraphicsPipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_VkSwapChainExtent.width);
	viewport.height = static_cast<float>(m_VkSwapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(m_VkCommandBuffers[m_CurrentFrame], 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_VkSwapChainExtent;
	vkCmdSetScissor(m_VkCommandBuffers[m_CurrentFrame], 0, 1, &scissor);
}

void GraphicsAPI::EndFrame()
{
	vkCmdEndRenderPass(m_VkCommandBuffers[m_CurrentFrame]);
	HandleVkResult(vkEndCommandBuffer(m_VkCommandBuffers[m_CurrentFrame]));

	const VkSemaphore waitSemaphores[]
	{
		m_VkImageAvailableSemaphores[m_CurrentFrame]
	};

	constexpr VkPipelineStageFlags waitStages[]
	{
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	const VkSemaphore signalSemaphores[]
	{
		m_VkRenderFinishedSemaphores[m_CurrentFrame]
	};

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_VkCommandBuffers[m_CurrentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	HandleVkResult(vkQueueSubmit(m_VkGraphicsQueue, 1, &submitInfo, m_VkInFlightFences[m_CurrentFrame]));

	const VkSwapchainKHR swapChains[]
	{
		m_VkSwapChain
	};

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &m_CurrentFrameSwapchainImageIndex;
	presentInfo.pResults = nullptr; // Optional

	const auto vkResult{ vkQueuePresentKHR(m_VkPresentQueue, &presentInfo) };

	if (vkResult == VK_ERROR_OUT_OF_DATE_KHR || vkResult == VK_SUBOPTIMAL_KHR)
		RecreateSwapchain();

	else if (vkResult != VK_SUCCESS)
		HandleVkResult(vkResult);

	m_CurrentFrame = (m_CurrentFrame + 1) % sk_MaxFramesInFlight;
}

void GraphicsAPI::DrawMesh(uint32_t meshDataID, uint32_t /*materialID*/, const XMMATRIX& transform) const
{
	const auto& resourceManager{ ResourceManager::Get() };

	const auto& meshData{ resourceManager.GetMeshData(meshDataID) };

	const VkBuffer vertexBuffers[]
	{
		meshData.m_VertexBuffer
	};

	constexpr VkDeviceSize offsets[]
	{
		0
	};

	vkCmdBindVertexBuffers(m_VkCommandBuffers[m_CurrentFrame], 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(m_VkCommandBuffers[m_CurrentFrame], meshData.m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(m_VkCommandBuffers[m_CurrentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_VkGraphicsPipelineLayout, 0, 1, &m_VkDescriptorSets[m_CurrentFrame], 0, nullptr);

	vkCmdDrawIndexed(m_VkCommandBuffers[m_CurrentFrame], meshData.m_IndexCount, 1, 0, 0, 0);
}

VkSurfaceFormatKHR GraphicsAPI::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats)
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return availableFormat;

	return availableFormats[0];
}

VkPresentModeKHR GraphicsAPI::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes)
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return availablePresentMode;

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D GraphicsAPI::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
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

void GraphicsAPI::CreateSwapchain()
{
	const SwapChainSupportDetails swapChainSupport{ QuerySwapChainSupport(m_VkPhysicalDevice) };

	const VkSurfaceFormatKHR surfaceFormat{ ChooseSwapSurfaceFormat(swapChainSupport.m_Formats) };
	const VkPresentModeKHR presentMode{ ChooseSwapPresentMode(swapChainSupport.m_PresentModes) };
	const VkExtent2D extent{ ChooseSwapExtent(swapChainSupport.m_Capabilities) };

	uint32_t imageCount{ swapChainSupport.m_Capabilities.minImageCount + 1 };

	if (swapChainSupport.m_Capabilities.maxImageCount > 0 && imageCount > swapChainSupport.m_Capabilities.maxImageCount)
		imageCount = swapChainSupport.m_Capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_VkSurface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	const uint32_t queueFamilyIndices[]
	{
		m_QueueFamilyIndices.m_GraphicsFamily.value(),
		m_QueueFamilyIndices.m_PresentFamily.value()
	};

	if (m_QueueFamilyIndices.m_GraphicsFamily != m_QueueFamilyIndices.m_PresentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	createInfo.preTransform = swapChainSupport.m_Capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	HandleVkResult(vkCreateSwapchainKHR(m_VkDevice, &createInfo, nullptr, &m_VkSwapChain));

	vkGetSwapchainImagesKHR(m_VkDevice, m_VkSwapChain, &imageCount, nullptr);
	m_VkSwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_VkDevice, m_VkSwapChain, &imageCount, m_VkSwapChainImages.data());

	m_VkSwapChainImageFormat = surfaceFormat.format;
	m_VkSwapChainExtent = extent;
}

void GraphicsAPI::CreateSwapchainImageViews()
{
	m_VkSwapChainImageViews.resize(m_VkSwapChainImages.size());

	for (size_t i{}; i < m_VkSwapChainImages.size(); ++i)
		m_VkSwapChainImageViews[i] = CreateImageView(m_VkSwapChainImages[i], m_VkSwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void GraphicsAPI::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_VkSwapChainImageFormat;
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

	HandleVkResult(vkCreateRenderPass(m_VkDevice, &renderPassInfo, nullptr, &m_VkRenderPass));
}

std::vector<char> GraphicsAPI::ReadShaderFile(const std::wstring& filename)
{
	std::ifstream file{ filename, std::ios::ate | std::ios::binary };

	if (!file.is_open())
		Logger::Get().LogError(L"Unable to open " + filename);

	const size_t fileSize{ static_cast<size_t>(file.tellg()) };
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

VkShaderModule GraphicsAPI::CreateShaderModule(const std::vector<char>& code) const
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	HandleVkResult(vkCreateShaderModule(m_VkDevice, &createInfo, nullptr, &shaderModule));

	return shaderModule;
}

void GraphicsAPI::CreateGraphicsPipeline()
{
	const auto vertShaderCode{ ReadShaderFile(L"Shaders/TestTrianglesTextured_VS.spv") };
	const auto fragShaderCode{ ReadShaderFile(L"Shaders/TestTrianglesTextured_PS.spv") };

	const VkShaderModule vertShaderModule{ CreateShaderModule(vertShaderCode) };
	const VkShaderModule fragShaderModule{ CreateShaderModule(fragShaderCode) };

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "VSMain";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "PSMain";

	VkPipelineShaderStageCreateInfo shaderStages[]
	{
		vertShaderStageInfo,
		fragShaderStageInfo
	};

	const std::vector<VkDynamicState> dynamicStates
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	const auto& bindingDescription = Vertex::GetBindingDescription();
	const auto& attributeDescriptions = Vertex::GetAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_VkSwapChainExtent.width);
	viewport.height = static_cast<float>(m_VkSwapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_VkSwapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_VkDescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	HandleVkResult(vkCreatePipelineLayout(m_VkDevice, &pipelineLayoutInfo, nullptr, &m_VkGraphicsPipelineLayout));

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = m_VkGraphicsPipelineLayout;
	pipelineInfo.renderPass = m_VkRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	HandleVkResult(vkCreateGraphicsPipelines(m_VkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_VkGraphicsPipeline));

	vkDestroyShaderModule(m_VkDevice, fragShaderModule, nullptr);
	vkDestroyShaderModule(m_VkDevice, vertShaderModule, nullptr);
}

void GraphicsAPI::CreateFrameBuffers()
{
	m_VkFrameBuffers.resize(m_VkSwapChainImageViews.size());

	for (size_t i{}; i < m_VkSwapChainImageViews.size(); ++i) {
		const std::array<VkImageView, 2> attachments
		{
			m_VkSwapChainImageViews[i],
			m_VkDepthImageView
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_VkRenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = m_VkSwapChainExtent.width;
		framebufferInfo.height = m_VkSwapChainExtent.height;
		framebufferInfo.layers = 1;

		HandleVkResult(vkCreateFramebuffer(m_VkDevice, &framebufferInfo, nullptr, &m_VkFrameBuffers[i]));
	}
}

void GraphicsAPI::CreateCommandBuffer()
{
	m_VkCommandBuffers.resize(sk_MaxFramesInFlight);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_VkCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = sk_MaxFramesInFlight;

	HandleVkResult(vkAllocateCommandBuffers(m_VkDevice, &allocInfo, m_VkCommandBuffers.data()));
}

void GraphicsAPI::CreateSyncObjects()
{
	m_VkImageAvailableSemaphores.resize(sk_MaxFramesInFlight);
	m_VkRenderFinishedSemaphores.resize(sk_MaxFramesInFlight);
	m_VkInFlightFences.resize(sk_MaxFramesInFlight);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i{}; i < sk_MaxFramesInFlight; ++i)
	{
		HandleVkResult(vkCreateSemaphore(m_VkDevice, &semaphoreInfo, nullptr, &m_VkImageAvailableSemaphores[i]));
		HandleVkResult(vkCreateSemaphore(m_VkDevice, &semaphoreInfo, nullptr, &m_VkRenderFinishedSemaphores[i]));
		HandleVkResult(vkCreateFence(m_VkDevice, &fenceInfo, nullptr, &m_VkInFlightFences[i]));
	}
}

void GraphicsAPI::CleanupSwapchain() const
{
	vkDestroyImageView(m_VkDevice, m_VkDepthImageView, nullptr);
	vkDestroyImage(m_VkDevice, m_VkDepthImage, nullptr);
	vkFreeMemory(m_VkDevice, m_VkDepthImageMemory, nullptr);

	for (const auto framebuffer : m_VkFrameBuffers)
		vkDestroyFramebuffer(m_VkDevice, framebuffer, nullptr);

	for (const auto imageView : m_VkSwapChainImageViews)
		vkDestroyImageView(m_VkDevice, imageView, nullptr);

	vkDestroySwapchainKHR(m_VkDevice, m_VkSwapChain, nullptr);
}

void GraphicsAPI::RecreateSwapchain()
{
	vkDeviceWaitIdle(m_VkDevice);

	CleanupSwapchain();

	CreateSwapchain();
	CreateSwapchainImageViews();
	CreateDepthResources();
	CreateFrameBuffers();
}

void GraphicsAPI::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	const std::array<VkDescriptorSetLayoutBinding, 2> bindings
	{
		uboLayoutBinding,
		samplerLayoutBinding
	};

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	HandleVkResult(vkCreateDescriptorSetLayout(m_VkDevice, &layoutInfo, nullptr, &m_VkDescriptorSetLayout));
}

void GraphicsAPI::CreateUniformBuffers()
{
	constexpr VkDeviceSize bufferSize{ sizeof(UBO) };

	m_VkUniformBuffers.resize(sk_MaxFramesInFlight);
	m_VkUniformBuffersMemory.resize(sk_MaxFramesInFlight);
	m_VkUniformBuffersMapped.resize(sk_MaxFramesInFlight);

	for (size_t i{}; i < sk_MaxFramesInFlight; ++i)
	{
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_VkUniformBuffers[i], m_VkUniformBuffersMemory[i]);
		vkMapMemory(m_VkDevice, m_VkUniformBuffersMemory[i], 0, bufferSize, 0, &m_VkUniformBuffersMapped[i]);
	}
}

void GraphicsAPI::UpdateUniformBuffer(uint32_t currentFrame) const
{
	static auto startTime{ std::chrono::high_resolution_clock::now() };

	const auto currentTime{ std::chrono::high_resolution_clock::now() };
	const float time{ std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count() };

	UBO ubo{};
	XMStoreFloat4x4(&ubo.m_ModelMat, XMMatrixRotationY(time * XMConvertToRadians(90.0f)));
	static XMFLOAT3 eyePos{ 0.0f, 1.5f, -3.0f };
	static XMFLOAT3 focusPos{ 0.0f, 0.0f, 0.0f };
	static XMFLOAT3 upDir{ 0.0f, 1.0f, 0.0f };
	XMStoreFloat4x4(&ubo.m_ViewMat, XMMatrixLookAtLH(XMLoadFloat3(&eyePos), XMLoadFloat3(&focusPos), XMLoadFloat3(&upDir)));
	XMStoreFloat4x4(&ubo.m_ProjectionMat, XMMatrixMultiply(XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), static_cast<float>(m_VkSwapChainExtent.width) / static_cast<float>(m_VkSwapChainExtent.height), 0.1f, 10.0f), XMMatrixScaling(1.0f, -1.0f, 1.0f)));

	memcpy(m_VkUniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}

void GraphicsAPI::CreateDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(sk_MaxFramesInFlight);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(sk_MaxFramesInFlight);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(sk_MaxFramesInFlight);

	HandleVkResult(vkCreateDescriptorPool(m_VkDevice, &poolInfo, nullptr, &m_VkDescriptorPool));
}

void GraphicsAPI::CreateDescriptorSets()
{
	const std::vector<VkDescriptorSetLayout> layouts(sk_MaxFramesInFlight, m_VkDescriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_VkDescriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(sk_MaxFramesInFlight);
	allocInfo.pSetLayouts = layouts.data();

	m_VkDescriptorSets.resize(sk_MaxFramesInFlight);

	HandleVkResult(vkAllocateDescriptorSets(m_VkDevice, &allocInfo, m_VkDescriptorSets.data()));

	for (size_t i{}; i < sk_MaxFramesInFlight; ++i)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_VkUniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UBO);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_VkTestModelTextureImageView;
		imageInfo.sampler = m_VkTestModelTextureSampler;

		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = m_VkDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = m_VkDescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(m_VkDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void GraphicsAPI::CreateTextureImage()
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels{ stbi_load("Resources/Textures/viking_room.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha) };
	const VkDeviceSize imageSize{ static_cast<VkDeviceSize>(texWidth) * texHeight * 4 };

	if (!pixels)
		Logger::Get().LogError(L"failed to load texture image!");

	m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(m_VkDevice, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, imageSize);
	vkUnmapMemory(m_VkDevice, stagingBufferMemory);

	stbi_image_free(pixels);

	CreateImage(texWidth, texHeight, m_MipLevels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VkTestModelTextureImage, m_VkTestModelTextureImageMemory);

	TransitionImageLayout(m_VkTestModelTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_MipLevels);
	CopyBufferToImage(stagingBuffer, m_VkTestModelTextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	GenerateMipmaps(m_VkTestModelTextureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, m_MipLevels);

	vkDestroyBuffer(m_VkDevice, stagingBuffer, nullptr);
	vkFreeMemory(m_VkDevice, stagingBufferMemory, nullptr);
}

void GraphicsAPI::CreateTextureImageView()
{
	m_VkTestModelTextureImageView = CreateImageView(m_VkTestModelTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, m_MipLevels);
}

void GraphicsAPI::CreateTextureSampler()
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = m_VkPhysicalDeviceProperties.limits.maxSamplerAnisotropy; //TODO: Use anisotropic level setting
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = static_cast<float>(m_MipLevels);

	HandleVkResult(vkCreateSampler(m_VkDevice, &samplerInfo, nullptr, &m_VkTestModelTextureSampler));
}

void GraphicsAPI::CreateDepthResources()
{
	const VkFormat depthFormat{ FindDepthFormat() };

	CreateImage(m_VkSwapChainExtent.width, m_VkSwapChainExtent.height, 1, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VkDepthImage, m_VkDepthImageMemory);
	m_VkDepthImageView = CreateImageView(m_VkDepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

VkFormat GraphicsAPI::FindDepthFormat() const
{
	return m_GfxDevice.FindSupportedFormat({ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void GraphicsAPI::GenerateMipmaps(VkImage image, VkFormat format, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) const
{
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(m_VkPhysicalDevice, format, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		Logger::Get().LogError(L"Image format does not support linear blitting for mip generation.");

	const VkCommandBuffer cmd{ BeginSingleTimeCmdBuffer() };

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth{ texWidth };
	int32_t mipHeight{ texHeight };

	for (uint32_t i{ 1 }; i < mipLevels; ++i)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		if (mipWidth > 1)
			mipWidth /= 2;

		if (mipHeight > 1)
			mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	EndSingleTimeCmdBuffer(cmd);
}

#endif
