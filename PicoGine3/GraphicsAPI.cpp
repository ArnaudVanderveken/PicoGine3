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

void GraphicsAPI::BeginFrame()
{
	
}

void GraphicsAPI::EndFrame()
{
	
}

void GraphicsAPI::DrawMesh(uint32_t meshDataID, uint32_t materialID, const XMMATRIX& transform) const
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
	m_pGfxSwapchain{ std::make_unique<GfxSwapchain>(this) },
	m_pGfxImmediateCommands{ std::make_unique<GfxImmediateCommands>(m_pGfxDevice.get(), "GraphicsAPI::m_pGfxImmediateCommands") },
	m_TimelineSemaphore{ m_pGfxDevice->CreateVkSemaphoreTimeline(m_pGfxSwapchain->GetImageCount() - 1, "GraphicsAPI::m_TimelineSemaphore") },
	m_pShaderModulePool{ std::make_unique<ShaderModulePool>(m_pGfxDevice.get()) }
{
	AcquireCommandBuffer();
	CreateDescriptorSetLayout();
	CreateGraphicsPipeline();
	CreateTextureImage();
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
	SubmitCommandBuffer();

	m_IsInitialized = true;
}

GraphicsAPI::~GraphicsAPI()
{
	const auto& device{ m_pGfxDevice->GetDevice() };
	vkDeviceWaitIdle(device);

	ResourceManager::Get().ReleaseGPUBuffers();
	m_pTestModelTextureImage.reset();

	vkDestroySemaphore(device, m_TimelineSemaphore, nullptr);

	vkDestroySampler(device, m_VkTestModelTextureSampler, nullptr);

	m_PerFrameUBO.clear();

	vkDestroyDescriptorPool(device, m_VkDescriptorPool, nullptr);

	vkDestroyDescriptorSetLayout(device, m_VkDescriptorSetLayout, nullptr);
	
	vkDestroyPipeline(device, m_VkGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, m_VkGraphicsPipelineLayout, nullptr);

	m_pShaderModulePool.reset();
	m_pGfxSwapchain.reset();

	WaitDeferredTasks();

	m_pGfxImmediateCommands.reset();
	m_pGfxDevice.reset();
}

bool GraphicsAPI::IsInitialized() const
{
	return m_IsInitialized;
}

GfxDevice* GraphicsAPI::GetGfxDevice() const
{
	return m_pGfxDevice.get();
}

GfxImmediateCommands* GraphicsAPI::GetGfxImmediateCommands() const
{
	return m_pGfxImmediateCommands.get();
}

VkSemaphore GraphicsAPI::GetTimelineSemaphore() const
{
	return m_TimelineSemaphore;
}

const GfxCommandBuffer& GraphicsAPI::GetCurrentCommandBuffer() const
{
	return m_CurrentCommandBuffer;
}

void GraphicsAPI::BeginFrame()
{
	AcquireCommandBuffer();
	const VkCommandBuffer cmdBuffer{ m_CurrentCommandBuffer.GetCmdBuffer() };

	UpdatePerFrameUBO();

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
	
	vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VkGraphicsPipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_pGfxSwapchain->Width());
	viewport.height = static_cast<float>(m_pGfxSwapchain->Height());
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_pGfxSwapchain->GetSwapChainExtent();
	
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
}

void GraphicsAPI::EndFrame()
{
	const auto cmdBuffer{ m_CurrentCommandBuffer.GetCmdBuffer() };
	if (cmdBuffer == VK_NULL_HANDLE)
	{
		Logger::Get().LogError(L"No command buffer was acquired beforehand.");
		return;
	}

	vkCmdEndRenderPass(cmdBuffer);

	SubmitCommandBuffer(true);
}

void GraphicsAPI::DrawMesh(uint32_t meshDataID, uint32_t /*materialID*/, const XMFLOAT4X4& transform) const
{
	const auto& resourceManager{ ResourceManager::Get() };
	const auto& meshData{ resourceManager.GetMeshData(meshDataID) };
	const auto currentFrameIndex{ m_pGfxSwapchain->GetCurrentFrameIndex() % GfxSwapchain::sk_MaxFramesInFlight };

	const auto cmdBuffer{ m_CurrentCommandBuffer.GetCmdBuffer() };
	if (cmdBuffer == VK_NULL_HANDLE)
	{
		Logger::Get().LogError(L"No command buffer was acquired beforehand.");
		return;
	}

	const PushConstants pushConstants{ transform };

	const VkBuffer vertexBuffers[]
	{
		meshData.m_pVertexBuffer->GetBuffer()
	};

	constexpr VkDeviceSize offsets[]
	{
		0
	};

	vkCmdPushConstants(cmdBuffer, m_VkGraphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstants);

	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(cmdBuffer, meshData.m_pIndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VkGraphicsPipelineLayout, 0, 1, &m_VkDescriptorSets[currentFrameIndex], 0, nullptr);

	vkCmdDrawIndexed(cmdBuffer, meshData.m_IndexCount, 1, 0, 0, 0);
}

void GraphicsAPI::AcquireCommandBuffer()
{
	if (m_CurrentCommandBuffer.GetCmdBuffer() != VK_NULL_HANDLE)
		Logger::Get().LogError(L"Cannot acquire more than one command buffer simultaneously");

	m_CurrentCommandBuffer = GfxCommandBuffer(m_pGfxImmediateCommands.get());
}

SubmitHandle GraphicsAPI::SubmitCommandBuffer(bool present)
{
	const auto cmdBuffer{ m_CurrentCommandBuffer.GetCmdBuffer() };
	if (cmdBuffer == VK_NULL_HANDLE)
	{
		Logger::Get().LogError(L"No command buffer was acquired beforehand.");
		return {};
	}

	if (present)
	{
		const auto image{ m_pGfxSwapchain->AcquireImage() };
		image->TransitionLayout(cmdBuffer,
								VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
								VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS });

		const uint64_t signalValue{ m_pGfxSwapchain->GetCurrentFrameIndex() + m_pGfxSwapchain->GetImageCount() };
		m_pGfxSwapchain->SetCurrentFrameTimelineWaitValue(signalValue);
		m_pGfxImmediateCommands->SignalSemaphore(m_TimelineSemaphore, signalValue);
	}

	const SubmitHandle handle{ m_pGfxImmediateCommands->Submit(*m_CurrentCommandBuffer.GetBufferWrapper()) };

	if (present)
	{
		const auto result{ m_pGfxSwapchain->Present(m_pGfxImmediateCommands->AcquireLastSubmitSemaphore()) };
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
			m_pGfxSwapchain->RecreateSwapchain();

		else if (result != VK_SUCCESS)
			HandleVkResult(result);
	}

	ProcessDeferredTasks();

	m_CurrentCommandBuffer = GfxCommandBuffer{};

	return handle;
}

void GraphicsAPI::AddDeferredTask(std::packaged_task<void()>&& task, SubmitHandle handle)
{
	if (handle.Empty())
		handle = m_pGfxImmediateCommands->GetNextSubmitHandle();
	
	m_DeferredTasks.emplace_back(std::move(task), handle);
}

void GraphicsAPI::ProcessDeferredTasks()
{
	while (m_DeferredTasks.empty() && m_pGfxImmediateCommands->IsReady(m_DeferredTasks.front().m_Handle, true))
	{
		m_DeferredTasks.front().m_Task();
		m_DeferredTasks.pop_front();
	}
}

void GraphicsAPI::WaitDeferredTasks()
{
	for (auto& task : m_DeferredTasks)
	{
		m_pGfxImmediateCommands->Wait(task.m_Handle);
		task.m_Task();
	}

	m_DeferredTasks.clear();
}

void GraphicsAPI::CreateGraphicsPipeline()
{
	const auto& device{ m_pGfxDevice->GetDevice() };

	const ShaderModule vertShaderModule{ m_pShaderModulePool->GetShaderModule(L"Shaders/SimpleMeshTextured_VS.spv") };
	const ShaderModule fragShaderModule{ m_pShaderModulePool->GetShaderModule(L"Shaders/SimpleMeshTextured_PS.spv") };

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule.m_ShaderModule;
	vertShaderStageInfo.pName = "VSMain";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule.m_ShaderModule;
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

	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	//pushConstantRange.size = sizeof(PushConstants);
	pushConstantRange.size = vertShaderModule.m_PushConstantSize;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_VkDescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

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
	m_PerFrameUBO.resize(GfxSwapchain::sk_MaxFramesInFlight);

	for (uint32_t i{}; i < GfxSwapchain::sk_MaxFramesInFlight; ++i)
	{
		m_PerFrameUBO[i] = std::make_unique<GfxBuffer>(this, sizeof(PerFrameUBO), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		m_PerFrameUBO[i]->Map();
	}
}

void GraphicsAPI::UpdatePerFrameUBO() const
{
	const auto currentFrame{ m_pGfxSwapchain->GetCurrentFrameIndex() % GfxSwapchain::sk_MaxFramesInFlight };

	PerFrameUBO ubo{};

	static XMFLOAT3 eyePos{ 0.0f, 1.5f, -5.0f };
	static XMFLOAT3 focusPos{ 0.0f, 0.0f, 0.0f };
	static XMFLOAT3 upDir{ 0.0f, 1.0f, 0.0f };

	const auto viewMat{ XMMatrixLookAtLH(XMLoadFloat3(&eyePos), XMLoadFloat3(&focusPos), XMLoadFloat3(&upDir)) };
	const auto projMat{ XMMatrixMultiply(XMMatrixPerspectiveFovLH(XM_PIDIV4, m_pGfxSwapchain->AspectRatio(), 0.1f, 10.0f), XMMatrixScaling(1.0f, -1.0f, 1.0f)) };
	const auto viewProjMat{ viewMat * projMat };

	XMStoreFloat4x4(&ubo.m_ViewMat, viewMat);
	XMStoreFloat4x4(&ubo.m_ViewInvMat, XMMatrixInverse(nullptr, viewMat));
	XMStoreFloat4x4(&ubo.m_ProjMat, projMat);
	XMStoreFloat4x4(&ubo.m_ProjInvMat, XMMatrixInverse(nullptr, projMat));
	XMStoreFloat4x4(&ubo.m_ViewProjMat, viewProjMat);
	XMStoreFloat4x4(&ubo.m_ViewProjInvMat, XMMatrixInverse(nullptr, viewProjMat));

	m_PerFrameUBO[currentFrame]->WriteToBuffer(&ubo);
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
		bufferInfo.buffer = m_PerFrameUBO[i]->GetBuffer();
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(PerFrameUBO);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_pTestModelTextureImage->m_ImageView;
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
	const auto cmdBuffer{ m_CurrentCommandBuffer.GetCmdBuffer() };

	int texWidth, texHeight, texChannels;
	stbi_uc* pixels{ stbi_load("Resources/Textures/viking_room.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha) };
	const VkDeviceSize imageSize{ static_cast<VkDeviceSize>(texWidth) * texHeight * 4 };

	if (!pixels)
		Logger::Get().LogError(L"failed to load texture image!");

	const uint32_t mipLevels{ static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1 };

	GfxBuffer stagingBuffer{ this, imageSize, 1, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
	stagingBuffer.Map();
	stagingBuffer.WriteToBuffer(pixels);
	stagingBuffer.Unmap();

	stbi_image_free(pixels);

	m_pTestModelTextureImage = std::make_unique<GfxImage>(this);
	m_pTestModelTextureImage->m_VkUsageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	m_pTestModelTextureImage->m_VkExtent = VkExtent3D{ static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1 };
	m_pTestModelTextureImage->m_VkType = VK_IMAGE_TYPE_2D;
	m_pTestModelTextureImage->m_VkImageFormat = VK_FORMAT_R8G8B8A8_SRGB;
	m_pTestModelTextureImage->m_NumLevels = mipLevels;
	vkGetPhysicalDeviceFormatProperties(m_pGfxDevice->GetPhysicalDevice(), m_pTestModelTextureImage->m_VkImageFormat, &m_pTestModelTextureImage->m_VkFormatProperties);

	m_pGfxDevice->CreateImage(m_pTestModelTextureImage->m_VkExtent.width,
							  m_pTestModelTextureImage->m_VkExtent.height,
							  m_pTestModelTextureImage->m_NumLevels,
							  m_pTestModelTextureImage->m_VkImageFormat,
							  VK_IMAGE_TILING_OPTIMAL,
							  m_pTestModelTextureImage->m_VkUsageFlags,
							  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
							  m_pTestModelTextureImage->m_VkImage,
							  m_pTestModelTextureImage->m_VkMemory[0]);

	m_pTestModelTextureImage->m_ImageView = m_pTestModelTextureImage->CreateImageView(VK_IMAGE_VIEW_TYPE_2D,
																					  m_pTestModelTextureImage->m_VkImageFormat,
																					  VK_IMAGE_ASPECT_COLOR_BIT,
																					  0,
																					  m_pTestModelTextureImage->m_NumLevels);

	m_pTestModelTextureImage->TransitionLayout(cmdBuffer,
											   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
											   VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS });

	m_pGfxDevice->CopyBufferToImage(cmdBuffer, 
									stagingBuffer.GetBuffer(),
									m_pTestModelTextureImage->m_VkImage,
									m_pTestModelTextureImage->m_VkExtent.width,
									m_pTestModelTextureImage->m_VkExtent.height);

	m_pTestModelTextureImage->TransitionLayout(cmdBuffer,
											   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
											   VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS });

	m_pTestModelTextureImage->GenerateMipmap(cmdBuffer);

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
	samplerInfo.maxLod = static_cast<float>(m_pTestModelTextureImage->m_NumLevels);

	HandleVkResult(vkCreateSampler(device, &samplerInfo, nullptr, &m_VkTestModelTextureSampler));
}

#endif
