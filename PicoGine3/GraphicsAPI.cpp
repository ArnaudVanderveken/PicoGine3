#include "pch.h"
#include "GraphicsAPI.h"

#include "ResourceManager.h"
#include "TimeManager.h"
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
	m_pGfxDevice{ std::make_unique<GfxDevice>() },
	m_pGfxSwapchain{ std::make_unique<GfxSwapchain>(m_pGfxDevice.get()) }
{
	CreateDescriptorSetLayout();
	CreateGraphicsPipeline();
	CreateTextureImage();
	CreateTextureImageView();
	CreateTextureSampler();
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
	CreateCommandBuffer();

	m_IsInitialized = true;
}

GraphicsAPI::~GraphicsAPI()
{
	const auto& device{ m_pGfxDevice->GetDevice() };
	vkDeviceWaitIdle(device);

	vkDestroySampler(device, m_VkTestModelTextureSampler, nullptr);
	vkDestroyImageView(device, m_VkTestModelTextureImageView, nullptr);
	vkDestroyImage(device, m_VkTestModelTextureImage, nullptr);
	vkFreeMemory(device, m_VkTestModelTextureImageMemory, nullptr);

	for (size_t i{}; i < GfxSwapchain::sk_MaxFramesInFlight; ++i)
	{
		vkDestroyBuffer(device, m_VkUniformBuffers[i], nullptr);
		vkFreeMemory(device, m_VkUniformBuffersMemory[i], nullptr);
	}

	vkDestroyDescriptorPool(device, m_VkDescriptorPool, nullptr);

	vkDestroyDescriptorSetLayout(device, m_VkDescriptorSetLayout, nullptr);
	
	vkDestroyPipeline(device, m_VkGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, m_VkGraphicsPipelineLayout, nullptr);
}

bool GraphicsAPI::IsInitialized() const
{
	return m_IsInitialized;
}

void GraphicsAPI::CreateVertexBuffer(const void* pBufferData, VkDeviceSize bufferSize, VkBuffer& outBuffer, VkDeviceMemory& outMemory) const
{
	const auto& device = m_pGfxDevice->GetDevice();
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	m_pGfxDevice->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, pBufferData, bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	m_pGfxDevice->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, outBuffer, outMemory);

	m_pGfxDevice->CopyBuffer(stagingBuffer, outBuffer, bufferSize);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void GraphicsAPI::CreateIndexBuffer(const void* pBufferData, VkDeviceSize bufferSize, VkBuffer& outBuffer, VkDeviceMemory& outMemory) const
{
	const auto& device = m_pGfxDevice->GetDevice();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	m_pGfxDevice->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, pBufferData, bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	m_pGfxDevice->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, outBuffer, outMemory);

	m_pGfxDevice->CopyBuffer(stagingBuffer, outBuffer, bufferSize);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void GraphicsAPI::BeginFrame() const
{
	const auto currentFrameIndex{ m_pGfxSwapchain->GetCurrentFrameIndex() };

	const auto vkResult{ m_pGfxSwapchain->AcquireNextImage() };

	if (vkResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		m_pGfxSwapchain->RecreateSwapchain();
		return;
	}

	if (vkResult != VK_SUCCESS && vkResult != VK_SUBOPTIMAL_KHR)
		HandleVkResult(vkResult);

	m_pGfxSwapchain->ResetFrameInFlightFence();
	vkResetCommandBuffer(m_VkCommandBuffers[currentFrameIndex], 0);

	UpdateUniformBuffer();

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	vkBeginCommandBuffer(m_VkCommandBuffers[currentFrameIndex], &beginInfo);

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_pGfxSwapchain->GetRenderPass();
	renderPassInfo.framebuffer = m_pGfxSwapchain->GetCurrentFrameBuffer();
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_pGfxSwapchain->GetSwapChainExtent();
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(m_VkCommandBuffers[currentFrameIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(m_VkCommandBuffers[currentFrameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, m_VkGraphicsPipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_pGfxSwapchain->Width());
	viewport.height = static_cast<float>(m_pGfxSwapchain->Height());
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(m_VkCommandBuffers[currentFrameIndex], 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_pGfxSwapchain->GetSwapChainExtent();
	vkCmdSetScissor(m_VkCommandBuffers[currentFrameIndex], 0, 1, &scissor);
}

void GraphicsAPI::EndFrame() const
{
	const auto currentFrameIndex{ m_pGfxSwapchain->GetCurrentFrameIndex() };

	vkCmdEndRenderPass(m_VkCommandBuffers[currentFrameIndex]);
	HandleVkResult(vkEndCommandBuffer(m_VkCommandBuffers[currentFrameIndex]));

	const auto vkResult{ m_pGfxSwapchain->SubmitCommandBuffers(&m_VkCommandBuffers[currentFrameIndex], 1) };

	if (vkResult == VK_ERROR_OUT_OF_DATE_KHR || vkResult == VK_SUBOPTIMAL_KHR)
		m_pGfxSwapchain->RecreateSwapchain();

	else if (vkResult != VK_SUCCESS)
		HandleVkResult(vkResult);
}

void GraphicsAPI::DrawMesh(uint32_t meshDataID, uint32_t /*materialID*/, const XMMATRIX& /*transform*/) const
{
	const auto& resourceManager{ ResourceManager::Get() };
	const auto& meshData{ resourceManager.GetMeshData(meshDataID) };
	const auto currentFrameIndex{ m_pGfxSwapchain->GetCurrentFrameIndex() };

	const VkBuffer vertexBuffers[]
	{
		meshData.m_VertexBuffer
	};

	constexpr VkDeviceSize offsets[]
	{
		0
	};

	vkCmdBindVertexBuffers(m_VkCommandBuffers[currentFrameIndex], 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(m_VkCommandBuffers[currentFrameIndex], meshData.m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(m_VkCommandBuffers[currentFrameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, m_VkGraphicsPipelineLayout, 0, 1, &m_VkDescriptorSets[currentFrameIndex], 0, nullptr);

	vkCmdDrawIndexed(m_VkCommandBuffers[currentFrameIndex], meshData.m_IndexCount, 1, 0, 0, 0);
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
	const auto& device{ m_pGfxDevice->GetDevice() };

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	HandleVkResult(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));

	return shaderModule;
}

void GraphicsAPI::CreateGraphicsPipeline()
{
	const auto& device{ m_pGfxDevice->GetDevice() };

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

	const auto& bindingDescription = Vertex3D::GetBindingDescription();
	const auto& attributeDescriptions = Vertex3D::GetAttributeDescriptions();

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
	viewport.width = static_cast<float>(m_pGfxSwapchain->Width());
	viewport.height = static_cast<float>(m_pGfxSwapchain->Height());
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_pGfxSwapchain->GetSwapChainExtent();

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

	HandleVkResult(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_VkGraphicsPipelineLayout));

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
	pipelineInfo.renderPass = m_pGfxSwapchain->GetRenderPass();
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	HandleVkResult(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_VkGraphicsPipeline));

	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void GraphicsAPI::CreateCommandBuffer()
{
	const auto& device{ m_pGfxDevice->GetDevice() };
	const auto& commandPool{ m_pGfxDevice->GetCommandPool() };

	m_VkCommandBuffers.resize(GfxSwapchain::sk_MaxFramesInFlight);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = GfxSwapchain::sk_MaxFramesInFlight;

	HandleVkResult(vkAllocateCommandBuffers(device, &allocInfo, m_VkCommandBuffers.data()));
}

void GraphicsAPI::CreateDescriptorSetLayout()
{
	const auto& device{ m_pGfxDevice->GetDevice() };

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

	HandleVkResult(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_VkDescriptorSetLayout));
}

void GraphicsAPI::CreateUniformBuffers()
{
	const auto& device{ m_pGfxDevice->GetDevice() };

	constexpr VkDeviceSize bufferSize{ sizeof(UBO) };

	m_VkUniformBuffers.resize(GfxSwapchain::sk_MaxFramesInFlight);
	m_VkUniformBuffersMemory.resize(GfxSwapchain::sk_MaxFramesInFlight);
	m_VkUniformBuffersMapped.resize(GfxSwapchain::sk_MaxFramesInFlight);

	for (size_t i{}; i < GfxSwapchain::sk_MaxFramesInFlight; ++i)
	{
		m_pGfxDevice->CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_VkUniformBuffers[i], m_VkUniformBuffersMemory[i]);
		vkMapMemory(device, m_VkUniformBuffersMemory[i], 0, bufferSize, 0, &m_VkUniformBuffersMapped[i]);
	}
}

void GraphicsAPI::UpdateUniformBuffer() const
{
	const float time{ TimeManager::Get().GetTotalTime() };
	const uint32_t currentFrame{ m_pGfxSwapchain->GetCurrentFrameIndex() };

	UBO ubo{};
	XMStoreFloat4x4(&ubo.m_ModelMat, XMMatrixRotationY(time * XMConvertToRadians(90.0f)));
	static XMFLOAT3 eyePos{ 0.0f, 1.5f, -3.0f };
	static XMFLOAT3 focusPos{ 0.0f, 0.0f, 0.0f };
	static XMFLOAT3 upDir{ 0.0f, 1.0f, 0.0f };
	XMStoreFloat4x4(&ubo.m_ViewMat, XMMatrixLookAtLH(XMLoadFloat3(&eyePos), XMLoadFloat3(&focusPos), XMLoadFloat3(&upDir)));
	XMStoreFloat4x4(&ubo.m_ProjectionMat, XMMatrixMultiply(XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), m_pGfxSwapchain->AspectRatio(), 0.1f, 10.0f), XMMatrixScaling(1.0f, -1.0f, 1.0f)));

	memcpy(m_VkUniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}

void GraphicsAPI::CreateDescriptorPool()
{
	const auto& device{ m_pGfxDevice->GetDevice() };

	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(GfxSwapchain::sk_MaxFramesInFlight);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(GfxSwapchain::sk_MaxFramesInFlight);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(GfxSwapchain::sk_MaxFramesInFlight);

	HandleVkResult(vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_VkDescriptorPool));
}

void GraphicsAPI::CreateDescriptorSets()
{
	const auto& device{ m_pGfxDevice->GetDevice() };

	const std::vector<VkDescriptorSetLayout> layouts(GfxSwapchain::sk_MaxFramesInFlight, m_VkDescriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_VkDescriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(GfxSwapchain::sk_MaxFramesInFlight);
	allocInfo.pSetLayouts = layouts.data();

	m_VkDescriptorSets.resize(GfxSwapchain::sk_MaxFramesInFlight);

	HandleVkResult(vkAllocateDescriptorSets(device, &allocInfo, m_VkDescriptorSets.data()));

	for (size_t i{}; i < GfxSwapchain::sk_MaxFramesInFlight; ++i)
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

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void GraphicsAPI::CreateTextureImage()
{
	const auto& device{ m_pGfxDevice->GetDevice() };

	int texWidth, texHeight, texChannels;
	stbi_uc* pixels{ stbi_load("Resources/Textures/viking_room.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha) };
	const VkDeviceSize imageSize{ static_cast<VkDeviceSize>(texWidth) * texHeight * 4 };

	if (!pixels)
		Logger::Get().LogError(L"failed to load texture image!");

	m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	m_pGfxDevice->CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, imageSize);
	vkUnmapMemory(device, stagingBufferMemory);

	stbi_image_free(pixels);

	m_pGfxDevice->CreateImage(texWidth, texHeight, m_MipLevels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VkTestModelTextureImage, m_VkTestModelTextureImageMemory);

	m_pGfxDevice->TransitionImageLayout(m_VkTestModelTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_MipLevels);
	m_pGfxDevice->CopyBufferToImage(stagingBuffer, m_VkTestModelTextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	GenerateMipmaps(m_VkTestModelTextureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, m_MipLevels);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void GraphicsAPI::CreateTextureImageView()
{
	m_VkTestModelTextureImageView = m_pGfxDevice->CreateImageView(m_VkTestModelTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, m_MipLevels);
}

void GraphicsAPI::CreateTextureSampler()
{
	const auto& device{ m_pGfxDevice->GetDevice() };

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = m_pGfxDevice->GetPhysicalDeviceProperties().limits.maxSamplerAnisotropy; //TODO: Use anisotropic level setting
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = static_cast<float>(m_MipLevels);

	HandleVkResult(vkCreateSampler(device, &samplerInfo, nullptr, &m_VkTestModelTextureSampler));
}

void GraphicsAPI::GenerateMipmaps(VkImage image, VkFormat format, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) const
{
	const VkFormatProperties formatProperties{ m_pGfxDevice->GetFormatProperties(format) };

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		Logger::Get().LogError(L"Image format does not support linear blitting for mip generation.");

	const VkCommandBuffer cmd{ m_pGfxDevice->BeginSingleTimeCmdBuffer() };

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

	m_pGfxDevice->EndSingleTimeCmdBuffer(cmd);
}

#endif
