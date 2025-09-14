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
	const auto& vertexBuffer{ m_BuffersPool.Get(meshData.m_pVertexBufferHandle) };
	const auto& indexBuffer{ m_BuffersPool.Get(meshData.m_pIndexBufferHandle) };
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
		vertexBuffer->m_VkBuffer
	};

	constexpr VkDeviceSize offsets[]
	{
		0
	};

	vkCmdPushConstants(cmdBuffer, m_VkGraphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstants);

	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(cmdBuffer, indexBuffer->m_VkBuffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VkGraphicsPipelineLayout, 0, 1, &m_VkDescriptorSets[currentFrameIndex], 0, nullptr);

	vkCmdDrawIndexed(cmdBuffer, meshData.m_IndexCount, 1, 0, 0, 0);
}

void GraphicsAPI::AcquireCommandBuffer()
{
	if (m_CurrentCommandBuffer.GetCmdBuffer() != VK_NULL_HANDLE)
		Logger::Get().LogError(L"Cannot acquire more than one command buffer simultaneously");

	m_CurrentCommandBuffer = GfxCommandBuffer(this);
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

void GraphicsAPI::CheckAndUpdateDescriptorSets()
{
	if (!m_AwaitingDescriptorsCreation)
		return;

	//TODO
}

BufferHandle GraphicsAPI::AcquireBuffer(const BufferDesc& desc)
{
	VkBufferUsageFlags usageFlags{ (desc.m_Storage == StorageType_Device) ? VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT : 0u };

	if (desc.m_Usage == 0)
	{
		Logger::Get().LogError(L"Invalid buffer usage.");
		return {};
	}

	if (desc.m_Usage & BufferUsageBits_Index)
		usageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	
	if (desc.m_Usage & BufferUsageBits_Vertex)
		usageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	
	if (desc.m_Usage & BufferUsageBits_Uniform)
		usageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;

	if (desc.m_Usage & BufferUsageBits_Storage)
		usageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;

	if (desc.m_Usage & BufferUsageBits_Indirect)
		usageFlags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;

	if (desc.m_Usage & BufferUsageBits_ShaderBindingTable)
		usageFlags |= VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;

	if (desc.m_Usage & BufferUsageBits_AccelStructBuildInputReadOnly)
		usageFlags |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;

	if (desc.m_Usage & BufferUsageBits_AccelStructStorage)
		usageFlags |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;

	assert(usageFlags && L"Invalid buffer usage.");

	const VkMemoryPropertyFlags memFlags{ StorageTypeToVkMemoryPropertyFlags(desc.m_Storage) };

#define ENSURE_BUFFER_SIZE(flag, maxSize)											   \
    if (usageFlags & (flag))														   \
	{																				   \
		if (!desc.m_Size <= (maxSize))												   \
		{																			   \
			Logger::Get().LogError(std::format(L"Buffer size exceeds limit ", #flag)); \
			return {};                                                                 \
		}                                                                              \
	}

	const VkPhysicalDeviceLimits& limits{m_pGfxDevice->GetPhysicalDeviceProperties().limits };

	ENSURE_BUFFER_SIZE(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, limits.maxUniformBufferRange);
	ENSURE_BUFFER_SIZE(VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM, limits.maxStorageBufferRange);

#undef ENSURE_BUFFER_SIZE

	GfxBuffer buffer{
		.bufferSize_ = desc.m_Size,
		.vkUsageFlags_ = usageFlags,
		.vkMemFlags_ = memFlags,
		.m_pGraphicsAPI = this
	};

	const VkBufferCreateInfo ci{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = desc.m_Size,
		.usage = usageFlags,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
	};

	const auto& device{ m_pGfxDevice->GetDevice() };

	//if (VULKAN_USE_VMA)
	//{
	//	VmaAllocationCreateInfo vmaAllocInfo{};

	//	if (memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	//	{
	//		vmaAllocInfo{
	//			.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
	//			.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	//			.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
	//		};
	//	}

	//	if (memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	//	{
	//		// Check if coherent buffer is available.
	//		HandleVkResult(vkCreateBuffer(device, &ci, nullptr, &buffer.m_VkBuffer));
	//		VkMemoryRequirements requirements{};
	//		vkGetBufferMemoryRequirements(device, buffer.m_VkBuffer, &requirements);
	//		vkDestroyBuffer(device, buffer.m_VkBuffer, nullptr);
	//		buffer.m_VkBuffer = VK_NULL_HANDLE;

	//		if (requirements.memoryTypeBits & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
	//		{
	//			vmaAllocInfo.requiredFlags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	//			buffer.m_IsCoherentMemory = true;
	//		}
	//	}

	//	vmaAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;

	//	vmaCreateBuffer((VmaAllocator)getVmaAllocator(), &ci, &vmaAllocInfo, &buffer.m_VkBuffer, &buffer.m_VmaAllocation, nullptr);

	//	if (memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	//		vmaMapMemory((VmaAllocator)getVmaAllocator(), buffer.m_VmaAllocation, &buffer.m_pMappedPtr);
	//}
	//else
	//{
		HandleVkResult(vkCreateBuffer(device, &ci, nullptr, &buffer.m_VkBuffer));

		VkMemoryRequirements requirements{};
		vkGetBufferMemoryRequirements(device, buffer.m_VkBuffer, &requirements);
		if (requirements.memoryTypeBits & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
			buffer.m_IsCoherentMemory = true;

		constexpr VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
			.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR,
		};

		const VkMemoryAllocateInfo ai{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = &memoryAllocateFlagsInfo,
			.allocationSize = requirements.size,
			.memoryTypeIndex = m_pGfxDevice->FindMemoryType(requirements.memoryTypeBits, memFlags),
		};

		HandleVkResult(vkAllocateMemory(device, &ai, nullptr, &buffer.m_VkMemory));
		HandleVkResult(vkBindBufferMemory(device, buffer.m_VkBuffer, buffer.m_VkMemory, 0));

		if (memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
			HandleVkResult(vkMapMemory(device, buffer.m_VkMemory, 0, buffer.m_BufferSize, 0, &buffer.m_pMappedPtr));
	//}

	assert(buffer.m_VkBuffer != VK_NULL_HANDLE);

	HandleVkResult(m_pGfxDevice->SetVkObjectName(VK_OBJECT_TYPE_BUFFER, reinterpret_cast<uint64_t>(buffer.m_VkBuffer), desc.m_DebugName));

	if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
	{
		const VkBufferDeviceAddressInfo addressInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
			.buffer = buffer.m_VkBuffer,
		};
		buffer.m_VkDeviceAddress = vkGetBufferDeviceAddress(device, &addressInfo);
		assert(buffer.m_VkDeviceAddress);
	}

	return m_BuffersPool.Add(std::move(buffer));
}

void GraphicsAPI::Destroy(BufferHandle handle)
{
	GfxBuffer* buffer{ m_BuffersPool.Get(handle) };

	if (!buffer)
		return;

	//if (VULKAN_USE_VMA)
	//{
	//	if (buffer->m_pMappedPtr)
	//		vmaUnmapMemory((VmaAllocator)getVmaAllocator(), buf->vmaAllocation_);
	//	
	//	AddDeferredTask(std::packaged_task<void()>([vma = getVmaAllocator(), buffer = buffer->m_VkBuffer, allocation = buffer->m_VmaAllocation]()
	//	{
	//		vmaDestroyBuffer((VmaAllocator)vma, buffer, allocation);
	//	}));
	//}
	//else
	//{
		if (buffer->m_pMappedPtr)
			vkUnmapMemory(m_pGfxDevice->GetDevice(), buffer->m_VkMemory);
		
		AddDeferredTask(std::packaged_task<void()>([device = m_pGfxDevice->GetDevice(), buffer = buffer->m_VkBuffer, memory = buffer->m_VkMemory]()
		{
			vkDestroyBuffer(device, buffer, nullptr);
			vkFreeMemory(device, memory, nullptr);
		}));
	//}

	m_BuffersPool.Remove(handle);
}

GfxBuffer* GraphicsAPI::GetBuffer(BufferHandle handle) const
{
	return m_BuffersPool.Get(handle);
}

TextureHandle GraphicsAPI::AcquireTexture(const TextureDesc& desc)
{
	const VkFormat vkFormat{ FormatToVkFormat(desc.m_Format) };

	assert(vkFormat != VK_FORMAT_UNDEFINED && L"Invalid VkFormat value.");
	assert(desc.m_NumMipLevels && L"The number of mip levels specified must be greater than 0.");
	assert(!(desc.m_NumSamples > 1 && desc.m_NumMipLevels != 1) && L"The number of mip levels for multisampled images should be 1.");
	assert(!(desc.m_NumSamples > 1 && desc.m_Type == TextureType_3D) && L"Multisampled 3D images are not supported.");
	assert(desc.m_NumMipLevels <= CalculateMaxMipLevels(desc.m_Dimensions.m_Width, desc.m_Dimensions.m_Height) && L"The number of specified mip-levels is greater than the maximum possible number of mip-levels.");
	assert(desc.m_Usage && L"Texture usage flags are not set.");

	VkImageUsageFlags usageFlags = (desc.m_Storage == StorageType_Device) ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0;

	if (desc.m_Usage & TextureUsageBits_Sampled)
		usageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
	
	if (desc.m_Usage & TextureUsageBits_Storage)
	{
		assert(desc.m_NumSamples <= 1 && L"Storage images cannot be multisampled.");
		usageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
	}
	if (desc.m_Usage & TextureUsageBits_Attachment)
	{
		usageFlags |= sk_textureFormatProperties[desc.m_Format].m_Depth ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (desc.m_Storage == StorageType_Memoryless)
			usageFlags |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	}

	if (desc.m_Storage != StorageType_Memoryless)
		usageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	assert(usageFlags && "Invalid usage flags.");

	const VkMemoryPropertyFlags memoryFlags{ StorageTypeToVkMemoryPropertyFlags(desc.m_Storage) };

	const bool hasDebugName{ desc.m_DebugName && *desc.m_DebugName };

	char debugNameImage[256]{};
	char debugNameImageView[256]{};

	if (hasDebugName)
	{
		snprintf(debugNameImage, sizeof(debugNameImage) - 1, "Image: %s", desc.m_DebugName);
		snprintf(debugNameImageView, sizeof(debugNameImageView) - 1, "Image View: %s", desc.m_DebugName);
	}

	VkImageCreateFlags vkCreateFlags{};
	VkImageViewType vkImageViewType;
	VkImageType vkImageType;
	VkSampleCountFlagBits vkSamples{ VK_SAMPLE_COUNT_1_BIT };
	uint32_t numLayers{ desc.m_NumLayers };
	const VkExtent3D vkExtent{ desc.m_Dimensions.m_Width, desc.m_Dimensions.m_Height, desc.m_Dimensions.m_Depth };
	const uint32_t numLevels{ desc.m_NumMipLevels };

	const VkPhysicalDeviceLimits& limits{ m_pGfxDevice->GetPhysicalDeviceLimits() };

	switch (desc.m_Type)
	{
	case TextureType_2D:
		assert((vkExtent.width <= limits.maxImageDimension2D && vkExtent.height <= limits.maxImageDimension2D) && L"Texture 2D size exceeds limits.");
		vkImageViewType = numLayers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
		vkImageType = VK_IMAGE_TYPE_2D;
		const uint32_t maxSampleCount{ limits.framebufferColorSampleCounts & limits.framebufferDepthSampleCounts };
		vkSamples = GetVulkanSampleCountFlags(desc.m_NumSamples, maxSampleCount);
		break;
	case TextureType_3D:
		assert((vkExtent.width <= limits.maxImageDimension3D && vkExtent.height <= limits.maxImageDimension3D && vkExtent.depth <= limits.maxImageDimension3D) && L"Texture 3D size exceeds limits.");
		vkImageViewType = VK_IMAGE_VIEW_TYPE_3D;
		vkImageType = VK_IMAGE_TYPE_3D;
		break;
	case TextureType_Cube:
		vkImageViewType = numLayers > 1 ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
		vkImageType = VK_IMAGE_TYPE_2D;
		vkCreateFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		numLayers *= 6;
		break;
	}

	assert(numLevels > 0 && L"The image must contain at least one mip-level");
	assert(numLayers > 0 && L"The image must contain at least one layer");
	assert(vkSamples > 0 && L"The image must contain at least one sample");
	assert(vkExtent.width > 0);
	assert(vkExtent.height > 0);
	assert(vkExtent.depth > 0);

	GfxImage image{
		.m_VkUsageFlags = usageFlags,
		.m_VkExtent = vkExtent,
		.m_VkType = vkImageType,
		.m_VkImageFormat = vkFormat,
		.m_VkSamples = vkSamples,
		.m_NumLevels = numLevels,
		.m_NumLayers = numLayers,
		.m_IsDepthFormat = GfxImage::IsDepthFormat(vkFormat),
		.m_IsStencilFormat = GfxImage::IsStencilFormat(vkFormat),
		.m_pGraphicsAPI = this
	};

	if (hasDebugName)
		snprintf(image.m_DebugName, sizeof(image.m_DebugName) - 1, "%s", desc.m_DebugName);

	const VkImageCreateInfo ci{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = vkCreateFlags,
		.imageType = vkImageType,
		.format = vkFormat,
		.extent = vkExtent,
		.mipLevels = numLevels,
		.arrayLayers = numLayers,
		.samples = vkSamples,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = usageFlags,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	//if (VULKAN_USE_VMA)
	//{
	//	VmaAllocationCreateInfo vmaAllocInfo{
	//		.usage = memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ? VMA_MEMORY_USAGE_CPU_TO_GPU : VMA_MEMORY_USAGE_AUTO,
	//	};

	//	VkResult result{ vmaCreateImage((VmaAllocator)GetVmaAllocator(), &ci, &vmaAllocInfo, &image.m_VkImage, &image.m_VmaAllocation, nullptr) };

	//	if (!LVK_VERIFY(result == VK_SUCCESS))
	//	{
	//		Logger::Get().LogError(std::format(L"Failed to create vk image: {}, memflags: {},  imageformat: {}\n", result, memFlags, image.m_VkImageFormat));
	//		return {};
	//	}

	//	if (memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	//		vmaMapMemory((VmaAllocator)GetVmaAllocator(), image.m_VmaAllocation, &image.m_MappedPtr);
	//}
	//else
	//{
		const auto& device{ m_pGfxDevice->GetDevice() };
		HandleVkResult(vkCreateImage(device, &ci, nullptr, &image.m_VkImage));

		VkMemoryRequirements2 memRequirements{
			.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2
		};

		const VkImage img = image.m_VkImage;

		const VkImageMemoryRequirementsInfo2 imgRequirements{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
			.image = img
		};

		vkGetImageMemoryRequirements2(device, &imgRequirements, &memRequirements);

		const VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
			.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR,
		};

		const VkMemoryAllocateInfo allocateInfo{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = &memoryAllocateFlagsInfo,
			.allocationSize = memRequirements.memoryRequirements.size,
			.memoryTypeIndex = m_pGfxDevice->FindMemoryType(memRequirements.memoryRequirements.memoryTypeBits, memoryFlags),
		};

		vkAllocateMemory(device, &allocateInfo, nullptr, &image.m_VkMemory);

		const VkBindImageMemoryInfo bindInfo{
			.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
			.image = image.m_VkImage,
			.memory = image.m_VkMemory
		};
		HandleVkResult(vkBindImageMemory2(device, 1, &bindInfo));

		if (memoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
			HandleVkResult(vkMapMemory(device, image.m_VkMemory, 0, VK_WHOLE_SIZE, 0, &image.m_MappedPtr));
	//}

	HandleVkResult(m_pGfxDevice->SetVkObjectName(VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64_t>(image.m_VkImage), debugNameImage));

	vkGetPhysicalDeviceFormatProperties(m_pGfxDevice->GetPhysicalDevice(), image.m_VkImageFormat, &image.m_VkFormatProperties);

	VkImageAspectFlags aspect = 0;
	if (image.m_IsDepthFormat || image.m_IsStencilFormat)
	{
		if (image.m_IsDepthFormat)
			aspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
		
		else if (image.m_IsStencilFormat)
			aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else
		aspect = VK_IMAGE_ASPECT_COLOR_BIT;

	const VkComponentMapping mapping{
		.r = static_cast<VkComponentSwizzle>(desc.m_Swizzle.m_R),
		.g = static_cast<VkComponentSwizzle>(desc.m_Swizzle.m_G),
		.b = static_cast<VkComponentSwizzle>(desc.m_Swizzle.m_B),
		.a = static_cast<VkComponentSwizzle>(desc.m_Swizzle.m_A),
	};

	image.m_ImageView = image.CreateImageView(vkImageViewType, vkFormat, aspect, 0, VK_REMAINING_MIP_LEVELS, 0, numLayers, mapping, debugNameImageView);
	assert(image.m_ImageView != VK_NULL_HANDLE && L"Unable to create image view.");

	if (image.m_VkUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT)
	{
		// use identity swizzle for storage images
		image.m_ImageViewStorage = image.CreateImageView(vkImageViewType, vkFormat, aspect, 0, VK_REMAINING_MIP_LEVELS, 0, numLayers, {}, debugNameImageView);
		assert(image.m_ImageViewStorage != VK_NULL_HANDLE && L"Unable to create image view.");
	}

	m_AwaitingDescriptorsCreation = true;

	/*if (desc.m_Data)
	{
		assert(desc.m_Type == TextureType_2D || desc.m_Type == TextureType_Cube);
		assert(desc.m_DataNumMipLevels <= desc.m_NumMipLevels);
		const uint32_t numLayers = desc.m_Type == TextureType_Cube ? 6 : 1;

		Upload(handle, { .dimensions = desc.m_Dimensions, .numLayers = numLayers, .numMipLevels = desc.m_DataNumMipLevels }, desc.m_Data);
		
		if (desc.m_GenerateMipmaps)
			this->GenerateMipmap(handle);
		
	}*/

	return { m_TexturesPool.Add(std::move(image)) };
}

void GraphicsAPI::Destroy(TextureHandle handle)
{
	const auto& device{ GetGfxDevice()->GetDevice() };

	const GfxImage* image{ m_TexturesPool.Get(handle) };

	if (image->m_ImageView != VK_NULL_HANDLE)
	{
		AddDeferredTask(std::packaged_task<void()>([device = device, imageView = image->m_ImageView]()
		{
			vkDestroyImageView(device, imageView, nullptr);
		}));
	}

	if (image->m_ImageViewStorage != VK_NULL_HANDLE)
	{
		AddDeferredTask(std::packaged_task<void()>([device = device, imageView = image->m_ImageViewStorage]()
		{
			vkDestroyImageView(device, imageView, nullptr);
		}));
	}

	for (size_t i{}; i != GfxImage::sk_MaxMipLevels; ++i)
	{
		for (size_t j{}; j != GfxImage::sk_MaxArrayLayers; ++j)
		{
			VkImageView view{ image->m_ImageViewForFramebuffer[i][j] };
			if (view != VK_NULL_HANDLE)
			{
				AddDeferredTask(std::packaged_task<void()>([device = device, imageView = view]()
				{
					vkDestroyImageView(device, imageView, nullptr);
				}));
			}
		}
	}

	if (!image->m_IsOwningVkImage)
		return;

	/*if (VULKAN_USE_VMA)
	{
		if (tex->mappedPtr_)
			vmaUnmapMemory((VmaAllocator)GetVmaAllocator(), tex->m_VmaAllocation);

		m_pGraphicsAPI->AddDeferredTask(std::packaged_task<void()>([vma = GetVmaAllocator(), image = tex->m_VkImage, allocation = tex->m_VmaAllocation](
		{
			vmaDestroyImage((VmaAllocator)vma, image, allocation);
		}));
	}
	else
	{*/
	if (image->m_MappedPtr)
		vkUnmapMemory(device, image->m_VkMemory);

	AddDeferredTask(std::packaged_task<void()>([device = device, image = image->m_VkImage, memory = image->m_VkMemory]()
		{
			vkDestroyImage(device, image, nullptr);
			if (memory != VK_NULL_HANDLE)
				vkFreeMemory(device, memory, nullptr);
		}));
	/*}*/

	m_TexturesPool.Remove(handle);
}

GfxImage* GraphicsAPI::GetTexture(TextureHandle handle) const
{
	return m_TexturesPool.Get(handle);
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
		BufferDesc ubDesc{
			.m_Usage = BufferUsageBits_Uniform,
			.m_Storage = StorageType_HostVisible,
			.m_Size = sizeof(PerFrameUBO)
		};
		m_PerFrameUBO[i] = AcquireBuffer(ubDesc);
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

	const auto& currentFrameUBO{ m_BuffersPool.Get(m_PerFrameUBO[currentFrame]) };
	currentFrameUBO->WriteBufferData(0, sizeof(PerFrameUBO), &ubo);
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
		const auto& perFrameUBO{ m_BuffersPool.Get(m_PerFrameUBO[i]) };
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = perFrameUBO->m_VkBuffer;
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

	// TODO: Move staging buffer to separate pool
	const BufferDesc sbDesc{
		.m_Usage = BufferUsageBits_Storage,
		.m_Storage = StorageType_HostVisible,
		.m_Size = imageSize,
		.m_Data = pixels
	};

	stbi_image_free(pixels);

	const TextureDesc tDesc{
		.m_Type = TextureType_2D,
		.m_Format = R8G8B8A8_SRGB,
		.m_Dimensions = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1},
		.m_Usage = TextureUsageBits_Storage | TextureUsageBits_Sampled,
		.m_NumMipLevels = mipLevels,
		.m_Storage = StorageType_Device
	};
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
							  m_pTestModelTextureImage->m_VkMemory);

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

uint32_t GraphicsAPI::CalculateMaxMipLevels(uint32_t width, uint32_t height)
{
	uint32_t levels{};
	do
	{
		++levels;
	} while ((width | height) >> levels);

	return levels;
}

VkSampleCountFlagBits GraphicsAPI::GetVulkanSampleCountFlags(uint32_t numSamples, VkSampleCountFlags maxSamplesMask)
{
	if (numSamples <= 1 || VK_SAMPLE_COUNT_2_BIT > maxSamplesMask)
		return VK_SAMPLE_COUNT_1_BIT;
	
	if (numSamples <= 2 || VK_SAMPLE_COUNT_4_BIT > maxSamplesMask)
		return VK_SAMPLE_COUNT_2_BIT;
	
	if (numSamples <= 4 || VK_SAMPLE_COUNT_8_BIT > maxSamplesMask)
		return VK_SAMPLE_COUNT_4_BIT;
	
	if (numSamples <= 8 || VK_SAMPLE_COUNT_16_BIT > maxSamplesMask)
		return VK_SAMPLE_COUNT_8_BIT;
	
	if (numSamples <= 16 || VK_SAMPLE_COUNT_32_BIT > maxSamplesMask)
		return VK_SAMPLE_COUNT_16_BIT;
	
	if (numSamples <= 32 || VK_SAMPLE_COUNT_64_BIT > maxSamplesMask)
		return VK_SAMPLE_COUNT_32_BIT;
	
	return VK_SAMPLE_COUNT_64_BIT;
}

#endif
