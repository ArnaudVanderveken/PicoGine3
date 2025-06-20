#include "pch.h"
#include "GfxStructs.h"

#include "GraphicsAPI.h"

#pragma region GfxBuffer

GfxBuffer::GfxBuffer(GraphicsAPI* pGraphicsAPI, size_t elementStride, size_t elementCount, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryFlags, size_t minAlignmentOffset) :
	m_pGraphicsAPI{ pGraphicsAPI },
	m_pMappedMemory{ nullptr },
	m_VkDeviceAddress{ 0 },
	m_UsageFlags{ usageFlags },
	m_MemoryFlags{ memoryFlags },
	m_ElementStride{ elementStride },
	m_ElementCount{ elementCount }
{
	const auto& device{ m_pGraphicsAPI->GetGfxDevice() };

	m_AlignmentSize = GetAlignment(elementStride, minAlignmentOffset);
	m_BufferSize = elementCount * m_AlignmentSize;
	device->CreateBuffer(m_BufferSize, usageFlags, memoryFlags, m_Buffer, m_BufferMemory);
}

GfxBuffer::~GfxBuffer()
{
	const auto& device{ m_pGraphicsAPI->GetGfxDevice()->GetDevice() };

	Unmap();

	m_pGraphicsAPI->AddDeferredTask(std::packaged_task<void()>([device = device, buffer = m_Buffer, memory = m_BufferMemory]()
		{
			vkDestroyBuffer(device, buffer, nullptr);
			vkFreeMemory(device, memory, nullptr);
		}));

}

void GfxBuffer::Map(size_t size, size_t offset)
{
	const auto& device{ m_pGraphicsAPI->GetGfxDevice()->GetDevice() };

	const size_t mappedSize{ std::min(size, m_BufferSize) };
	HandleVkResult(vkMapMemory(device, m_BufferMemory, offset, mappedSize, 0, &m_pMappedMemory));
}

void GfxBuffer::Unmap()
{
	if (m_pMappedMemory)
	{
		const auto& device{ m_pGraphicsAPI->GetGfxDevice()->GetDevice() };

		vkUnmapMemory(device, m_BufferMemory);
		m_pMappedMemory = nullptr;
	}
}

size_t GfxBuffer::GetBufferSize() const
{
	return m_BufferSize;
}

void GfxBuffer::WriteToBuffer(const void* data, size_t size, size_t offset) const
{
	assert(m_pMappedMemory && L"Cannot write to unmapped buffer!");

	if (size == ~0ULL)
		memcpy(m_pMappedMemory, data, m_BufferSize);

	else
	{
		char* memOffset = static_cast<char*>(m_pMappedMemory);
		memOffset += offset;
		memcpy(memOffset, data, size);
	}
}

void GfxBuffer::Flush(size_t size, size_t offset) const
{
	const auto& device{ m_pGraphicsAPI->GetGfxDevice()->GetDevice() };

	VkMappedMemoryRange mappedRange{};
	mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedRange.memory = m_BufferMemory;
	mappedRange.offset = offset;
	mappedRange.size = size;
	HandleVkResult(vkFlushMappedMemoryRanges(device, 1, &mappedRange));
}

void GfxBuffer::Invalidate(size_t size, size_t offset) const
{
	const auto& device{ m_pGraphicsAPI->GetGfxDevice()->GetDevice() };

	VkMappedMemoryRange mappedRange{};
	mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedRange.memory = m_BufferMemory;
	mappedRange.offset = offset;
	mappedRange.size = size;
	HandleVkResult(vkInvalidateMappedMemoryRanges(device, 1, &mappedRange));
}

void GfxBuffer::WriteToIndex(const void* data, int index) const
{
	WriteToBuffer(data, m_ElementStride, index * m_AlignmentSize);
}

void GfxBuffer::FlushIndex(int index) const
{
	Flush(m_AlignmentSize, index * m_AlignmentSize);
}

void GfxBuffer::InvalidateIndex(int index) const
{
	Invalidate(m_AlignmentSize, index * m_AlignmentSize);
}

VkBuffer GfxBuffer::GetBuffer() const
{
	return m_Buffer;
}

VkDeviceMemory GfxBuffer::GetBufferMemory() const
{
	return m_BufferMemory;
}

size_t GfxBuffer::GetAlignment(size_t elementStride, size_t minAlignmentOffset)
{
	if (minAlignmentOffset > 0)
		return (elementStride + minAlignmentOffset - 1) & ~(minAlignmentOffset - 1);

	return elementStride;
}

#pragma endregion

#pragma region GfxImage

GfxImage::GfxImage(GraphicsAPI* pGraphicsAPI) :
	m_pGraphicsAPI{ pGraphicsAPI }
{
}

GfxImage::~GfxImage()
{
	const auto& device{ m_pGraphicsAPI->GetGfxDevice()->GetDevice() };

	if (m_ImageView != VK_NULL_HANDLE)
	{
		m_pGraphicsAPI->AddDeferredTask(std::packaged_task<void()>([device = device, imageView = m_ImageView]()
			{
				vkDestroyImageView(device, imageView, nullptr);
			}));
	}

	if (m_ImageViewStorage != VK_NULL_HANDLE)
	{
		m_pGraphicsAPI->AddDeferredTask(std::packaged_task<void()>([device = device, imageView = m_ImageViewStorage]()
			{
				vkDestroyImageView(device, imageView, nullptr);
			}));
	}

	for (size_t i{}; i != sk_MaxMipLevels; ++i)
	{
		for (size_t j{}; j != sk_MaxArrayLayers; ++j)
		{
			VkImageView view{ m_ImageViewForFramebuffer[i][j] };
			if (view != VK_NULL_HANDLE)
			{
				m_pGraphicsAPI->AddDeferredTask(std::packaged_task<void()>([device = device, imageView = view]()
					{
						vkDestroyImageView(device, imageView, nullptr);
					}));
			}
		}
	}

	if (!m_IsOwningVkImage)
		return;

	/*if (LVK_VULKAN_USE_VMA && tex->vkMemory_[1] == VK_NULL_HANDLE)
	{
		if (tex->mappedPtr_) {
			vmaUnmapMemory((VmaAllocator)getVmaAllocator(), tex->vmaAllocation_);
		}
		m_pGraphicsAPI->AddDeferredTask(std::packaged_task<void()>([vma = getVmaAllocator(), image = tex->vkImage_, allocation = tex->vmaAllocation_]() {
			vmaDestroyImage((VmaAllocator)vma, image, allocation);
			}));
	}
	else
	{*/
		if (m_MappedPtr)
			vkUnmapMemory(device, m_VkMemory[0]);

		m_pGraphicsAPI->AddDeferredTask(std::packaged_task<void()>([device = device, image = m_VkImage, memory0 = m_VkMemory[0], memory1 = m_VkMemory[1], memory2 = m_VkMemory[2]]()
		{
			vkDestroyImage(device, image, nullptr);
			if (memory0 != VK_NULL_HANDLE)
				vkFreeMemory(device, memory0, nullptr);

			if (memory1 != VK_NULL_HANDLE)
				vkFreeMemory(device, memory1, nullptr);

			if (memory2 != VK_NULL_HANDLE)
				vkFreeMemory(device, memory2, nullptr);

		}));
	/*}*/
}

GfxImage::GfxImage(GfxImage&& other) noexcept :
	m_VkImage{ other.m_VkImage },
	m_VkFormatProperties{ other.m_VkFormatProperties },
	//m_VmaAllocation{ other.m_VmaAllocation },
	m_VkExtent{ other.m_VkExtent },
	m_VkType{ other.m_VkType },
	m_VkImageFormat{ other.m_VkImageFormat },
	m_VkSamples{ other.m_VkSamples },
	m_MappedPtr{ other.m_MappedPtr },
	m_IsSwapchainImage{ other.m_IsSwapchainImage },
	m_IsOwningVkImage{ other.m_IsOwningVkImage },
	m_IsResolveAttachment{ other.m_IsResolveAttachment },
	m_NumLevels{ other.m_NumLevels },
	m_NumLayers{ other.m_NumLayers },
	m_IsDepthFormat{ other.m_IsDepthFormat },
	m_IsStencilFormat{ other.m_IsStencilFormat },
	m_CurrentVkImageLayout{ other.m_CurrentVkImageLayout },
	m_ImageView{ other.m_ImageView },
	m_ImageViewStorage{ other.m_ImageViewStorage },
	m_ImageViewForFramebuffer{ other.m_ImageViewStorage },
	m_pGraphicsAPI{ other.m_pGraphicsAPI }
{
	m_VkMemory[0] = other.m_VkMemory[0];
	m_VkMemory[1] = other.m_VkMemory[1];
	m_VkMemory[2] = other.m_VkMemory[2];

	memcpy(m_DebugName, other.m_DebugName, 256);

	other.m_VkImage = VK_NULL_HANDLE;
	other.m_VkMemory[0] = VK_NULL_HANDLE;
	other.m_VkMemory[1] = VK_NULL_HANDLE;
	other.m_VkMemory[2] = VK_NULL_HANDLE;
	//other.m_VmaAllocation = VK_NULL_HANDLE;
	other.m_MappedPtr = nullptr;
	other.m_IsOwningVkImage = false;
	other.m_ImageView = VK_NULL_HANDLE;
	other.m_ImageViewStorage = VK_NULL_HANDLE;
	memset(other.m_ImageViewForFramebuffer, 0, sizeof(VkImageView) * sk_MaxArrayLayers * sk_MaxMipLevels);
}

GfxImage& GfxImage::operator=(GfxImage&& other) noexcept
{
	m_VkImage = other.m_VkImage;
	m_VkUsageFlags = other.m_VkUsageFlags;
	m_VkMemory[0] = other.m_VkMemory[0];
	m_VkMemory[1] = other.m_VkMemory[1];
	m_VkMemory[2] = other.m_VkMemory[2];
	//m_VmaAllocation = other.m_VmaAllocation;
	m_VkFormatProperties = other.m_VkFormatProperties;
	m_VkExtent = other.m_VkExtent;
	m_VkType = other.m_VkType;
	m_VkImageFormat = other.m_VkImageFormat;
	m_VkSamples = other.m_VkSamples;
	m_MappedPtr = other.m_MappedPtr;
	m_IsSwapchainImage = other.m_IsSwapchainImage;
	m_IsOwningVkImage = other.m_IsOwningVkImage;
	m_IsResolveAttachment = other.m_IsResolveAttachment;
	m_NumLevels = other.m_NumLevels;
	m_NumLayers = other.m_NumLayers;
	m_IsDepthFormat = other.m_IsDepthFormat;
	m_IsStencilFormat = other.m_IsStencilFormat;
	memcpy(m_DebugName, other.m_DebugName, 256);
	m_CurrentVkImageLayout = other.m_CurrentVkImageLayout;
	m_ImageView = other.m_ImageView;
	m_ImageViewStorage = other.m_ImageViewStorage;
	memcpy(m_ImageViewForFramebuffer, other.m_ImageViewForFramebuffer, sizeof(VkImageView) * sk_MaxArrayLayers * sk_MaxMipLevels);

	other.m_VkImage = VK_NULL_HANDLE;
	other.m_VkMemory[0] = VK_NULL_HANDLE;
	other.m_VkMemory[1] = VK_NULL_HANDLE;
	other.m_VkMemory[2] = VK_NULL_HANDLE;
	//other.m_VmaAllocation = VK_NULL_HANDLE;
	other.m_MappedPtr = nullptr;
	other.m_IsOwningVkImage = false;
	other.m_ImageView = VK_NULL_HANDLE;
	other.m_ImageViewStorage = VK_NULL_HANDLE;
	memset(other.m_ImageViewForFramebuffer, 0, sizeof(VkImageView) * sk_MaxArrayLayers * sk_MaxMipLevels);

	return *this;
}

bool GfxImage::IsSampledImage() const
{
	return (m_VkUsageFlags & VK_IMAGE_USAGE_SAMPLED_BIT) > 0;
}

bool GfxImage::IsStorageImage() const
{
	return (m_VkUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT) > 0;
}

bool GfxImage::IsColorAttachment() const
{
	return (m_VkUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) > 0;
}

bool GfxImage::IsDepthAttachment() const
{
	return (m_VkUsageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) > 0;
}

bool GfxImage::IsAttachment() const
{
	return (m_VkUsageFlags & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) > 0;
}

StageAccess GfxImage::GetPipelineStageAccess(VkImageLayout layout)
{
	switch (layout)
	{
	case VK_IMAGE_LAYOUT_UNDEFINED:
		return StageAccess
		{
			.m_Stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
			.m_Access = VK_ACCESS_2_NONE,
		};
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		return StageAccess
		{
			.m_Stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			.m_Access = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		};
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		return StageAccess
		{
			.m_Stage = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
			.m_Access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		};
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		return StageAccess
		{
			.m_Stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT,
			.m_Access = VK_ACCESS_2_SHADER_READ_BIT,
		};
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		return StageAccess
		{
			.m_Stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			.m_Access = VK_ACCESS_2_TRANSFER_READ_BIT,
		};
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		return StageAccess
		{
			.m_Stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			.m_Access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
		};
	case VK_IMAGE_LAYOUT_GENERAL:
		return StageAccess
		{
			.m_Stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			.m_Access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT,
		};
	case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
		return StageAccess
		{
			.m_Stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			.m_Access = VK_ACCESS_2_NONE | VK_ACCESS_2_SHADER_WRITE_BIT,
		};
	default:
		Logger::Get().LogError(L"Unsupported image layout transition!");
		return StageAccess
		{
			.m_Stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			.m_Access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
		};
	}
}

void GfxImage::ImageMemoryBarrier(VkCommandBuffer cmdBuffer,
	VkImage image,
	StageAccess src,
	StageAccess dst,
	VkImageLayout oldImageLayout,
	VkImageLayout newImageLayout,
	VkImageSubresourceRange subresourceRange)
{
	const VkImageMemoryBarrier2 barrier
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = src.m_Stage,
		.srcAccessMask = src.m_Access,
		.dstStageMask = dst.m_Stage,
		.dstAccessMask = dst.m_Access,
		.oldLayout = oldImageLayout,
		.newLayout = newImageLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = subresourceRange,
	};

	const VkDependencyInfo depInfo
	{
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrier,
	};

	vkCmdPipelineBarrier2(cmdBuffer, &depInfo);
}

VkImageView GfxImage::CreateImageView(VkImageViewType type,
	VkFormat format,
	VkImageAspectFlags aspectMask,
	uint32_t baseLevel,
	uint32_t numLevels,
	uint32_t baseLayer,
	uint32_t numLayers,
	const VkComponentMapping mapping,
	const VkSamplerYcbcrConversionInfo* ycbcr,
	const char* debugName) const
{
	const auto& gfxDevice{ m_pGraphicsAPI->GetGfxDevice() };

	const VkImageViewCreateInfo createInfo
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = ycbcr,
		.image = m_VkImage,
		.viewType = type,
		.format = format,
		.components = mapping,
		.subresourceRange = { aspectMask, baseLevel, numLevels ? numLevels : m_NumLevels, baseLayer, numLayers },
	};

	VkImageView vkView{ VK_NULL_HANDLE };
	const auto& device{ gfxDevice->GetDevice() };

	HandleVkResult(vkCreateImageView(device, &createInfo, nullptr, &vkView));
	HandleVkResult(gfxDevice->SetVkObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, reinterpret_cast<uint64_t>(vkView), debugName));

	return vkView;
}

void GfxImage::GenerateMipmap(VkCommandBuffer cmdBuffer) const
{
	// Check if device supports downscaling for color or depth/stencil buffer based on image format
	{
		constexpr uint32_t formatFeatureMask{ VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT };

		const bool hardwareDownscalingSupported{ (m_VkFormatProperties.optimalTilingFeatures & formatFeatureMask) == formatFeatureMask };

		if (!hardwareDownscalingSupported)
		{
			Logger::Get().LogError(std::format(L"Doesn't support hardware downscaling of this image format: {}", StrUtils::cstr2stdwstr(string_VkFormat(m_VkImageFormat))));
			return;
		}
	}

	const VkFilter blitFilter{ !(m_IsDepthFormat || m_IsStencilFormat) && (m_VkFormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST };

	const VkImageAspectFlags imageAspectFlags{ GetImageAspectFlags() };

	if (vkCmdBeginDebugUtilsLabelEXT)
	{
		constexpr VkDebugUtilsLabelEXT debugLabel
		{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
			.pLabelName = "Generate mipmaps",
			.color = {1.0f, 0.75f, 1.0f, 1.0f},
		};
		vkCmdBeginDebugUtilsLabelEXT(cmdBuffer, &debugLabel);
	}

	const VkImageLayout originalImageLayout{ m_CurrentVkImageLayout };

	assert(originalImageLayout != VK_IMAGE_LAYOUT_UNDEFINED);

	// 0: Transition the first level and all layers into VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
	TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VkImageSubresourceRange{ imageAspectFlags, 0, 1, 0, m_NumLayers });

	for (uint32_t layer{}; layer < m_NumLayers; ++layer)
	{
		int32_t mipWidth = static_cast<int32_t>(m_VkExtent.width);
		int32_t mipHeight = static_cast<int32_t>(m_VkExtent.height);

		for (uint32_t level{ 1 }; level < m_NumLevels; ++level)
		{
			// 1: Transition the i-th level to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; it will be copied into from the (i-1)-th layer
			ImageMemoryBarrier(
				cmdBuffer,
				m_VkImage,
				StageAccess{ .m_Stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, .m_Access = VK_ACCESS_2_NONE },
				StageAccess{ .m_Stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .m_Access = VK_ACCESS_2_TRANSFER_WRITE_BIT },
				VK_IMAGE_LAYOUT_UNDEFINED, // oldImageLayout
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // newImageLayout
				VkImageSubresourceRange{ imageAspectFlags, level, 1, layer, 1 }
			);

			const int32_t nextLevelWidth{ mipWidth > 1 ? mipWidth / 2 : 1 };
			const int32_t nextLevelHeight{ mipHeight > 1 ? mipHeight / 2 : 1 };

			const VkOffset3D srcOffsets[2]
			{
				VkOffset3D{0, 0, 0},
				VkOffset3D{mipWidth, mipHeight, 1},
			};
			const VkOffset3D dstOffsets[2]
			{
				VkOffset3D{0, 0, 0},
				VkOffset3D{nextLevelWidth, nextLevelHeight, 1},
			};

			// 2: Blit the image from the prev mip-level (i-1) (VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) to the current mip-level (i)
			// (VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
			const VkImageBlit blit
			{
				.srcSubresource = VkImageSubresourceLayers{imageAspectFlags, level - 1, layer, 1},
				.srcOffsets = {srcOffsets[0], srcOffsets[1]},
				.dstSubresource = VkImageSubresourceLayers{imageAspectFlags, level, layer, 1},
				.dstOffsets = {dstOffsets[0], dstOffsets[1]},
			};
			vkCmdBlitImage(cmdBuffer,
				m_VkImage,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				m_VkImage,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&blit,
				blitFilter);
			// 3: Transition i-th level to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL as it will be read from in the next iteration
			ImageMemoryBarrier(
				cmdBuffer,
				m_VkImage,
				StageAccess{ .m_Stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .m_Access = VK_ACCESS_2_TRANSFER_WRITE_BIT },
				StageAccess{ .m_Stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .m_Access = VK_ACCESS_2_TRANSFER_READ_BIT },
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, /* oldImageLayout */
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, /* newImageLayout */
				VkImageSubresourceRange{ imageAspectFlags, level, 1, layer, 1 }
			);

			// Compute the size of the next mip-level
			mipWidth = nextLevelWidth;
			mipHeight = nextLevelHeight;
		}
	}

	// 4: Transition all levels and layers (faces) to their final layout
	ImageMemoryBarrier(
		cmdBuffer,
		m_VkImage,
		StageAccess{ .m_Stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .m_Access = VK_ACCESS_2_TRANSFER_READ_BIT },
		StageAccess{ .m_Stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, .m_Access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT },
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // oldImageLayout
		originalImageLayout, // newImageLayout
		VkImageSubresourceRange{ imageAspectFlags, 0, m_NumLevels, 0, m_NumLayers }
	);

	if (vkCmdEndDebugUtilsLabelEXT)
		vkCmdEndDebugUtilsLabelEXT(cmdBuffer);

	m_CurrentVkImageLayout = originalImageLayout;
}

void GfxImage::TransitionLayout(VkCommandBuffer cmdBuffer, VkImageLayout newImageLayout, const VkImageSubresourceRange& subresourceRange) const
{
	const VkImageLayout oldImageLayout
	{
		m_CurrentVkImageLayout == VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL
		? (IsDepthAttachment() ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		: m_CurrentVkImageLayout
	};

	if (newImageLayout == VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL)
		newImageLayout = IsDepthAttachment() ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	StageAccess src{ GetPipelineStageAccess(oldImageLayout) };
	StageAccess dst{ GetPipelineStageAccess(newImageLayout) };

	if (IsDepthAttachment() && m_IsResolveAttachment)
	{
		// https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#renderpass-resolve-operations
		src.m_Stage |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		dst.m_Stage |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		src.m_Access |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		dst.m_Access |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	}

	ImageMemoryBarrier(
		cmdBuffer,
		m_VkImage,
		src,
		dst,
		m_CurrentVkImageLayout,
		newImageLayout,
		subresourceRange
	);

	m_CurrentVkImageLayout = newImageLayout;
}

VkImageAspectFlags GfxImage::GetImageAspectFlags() const
{
	VkImageAspectFlags flags{};

	flags |= m_IsDepthFormat ? VK_IMAGE_ASPECT_DEPTH_BIT : 0;
	flags |= m_IsStencilFormat ? VK_IMAGE_ASPECT_STENCIL_BIT : 0;
	flags |= !(m_IsDepthFormat || m_IsStencilFormat) ? VK_IMAGE_ASPECT_COLOR_BIT : 0;

	return flags;
}

VkImageView GfxImage::GetOrCreateVkImageViewForFramebuffer(uint8_t level, uint16_t layer)
{
	assert(level < sk_MaxMipLevels);
	assert(layer < sk_MaxArrayLayers);

	if (level >= sk_MaxMipLevels || layer >= sk_MaxArrayLayers)
		return VK_NULL_HANDLE;

	if (m_ImageViewForFramebuffer[level][layer] != VK_NULL_HANDLE)
		return m_ImageViewForFramebuffer[level][layer];


	char debugNameImageView[256]{ 0 };
	snprintf(debugNameImageView, sizeof(debugNameImageView) - 1, "Image View: '%s' imageViewForFramebuffer_[%u][%u]", m_DebugName, level, layer);

	m_ImageViewForFramebuffer[level][layer] = CreateImageView(VK_IMAGE_VIEW_TYPE_2D,
		m_VkImageFormat,
		GetImageAspectFlags(),
		level,
		1u,
		layer,
		1u,
		{},
		nullptr,
		debugNameImageView);

	return m_ImageViewForFramebuffer[level][layer];
}

bool GfxImage::IsDepthFormat(VkFormat format)
{
	return (format == VK_FORMAT_D16_UNORM) || (format == VK_FORMAT_X8_D24_UNORM_PACK32) || (format == VK_FORMAT_D32_SFLOAT) ||
		(format == VK_FORMAT_D16_UNORM_S8_UINT) || (format == VK_FORMAT_D24_UNORM_S8_UINT) || (format == VK_FORMAT_D32_SFLOAT_S8_UINT);
}

bool GfxImage::IsStencilFormat(VkFormat format)
{
	return (format == VK_FORMAT_S8_UINT) || (format == VK_FORMAT_D16_UNORM_S8_UINT) || (format == VK_FORMAT_D24_UNORM_S8_UINT) ||
		(format == VK_FORMAT_D32_SFLOAT_S8_UINT);
}


#pragma endregion
