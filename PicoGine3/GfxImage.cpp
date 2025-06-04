#include "pch.h"
#include "GfxImage.h"

#include <format>

#include "GraphicsAPI.h"

GfxImage::GfxImage(GraphicsAPI* pGraphicsAPI) :
	m_pGraphicsAPI{ pGraphicsAPI }
{
}

GfxImage::~GfxImage()
{
	const auto& device{ m_pGraphicsAPI->GetGfxDevice()->GetDevice() };

	m_pGraphicsAPI->AddDeferredTask(std::packaged_task<void()>([device = device, imageView = m_ImageView]()
	{
		vkDestroyImageView(device, imageView, nullptr);
	}));

	if (m_ImageViewStorage)
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

	if (m_IsOwningVkImage)
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

VkImageView GfxImage::CreateImageView(GfxDevice* pGfxDevice,
                                      VkImageViewType type,
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
	const auto& device{ pGfxDevice->GetDevice() };

	HandleVkResult(vkCreateImageView(device, &createInfo, nullptr, &vkView));
	HandleVkResult(pGfxDevice->SetVkObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, reinterpret_cast<uint64_t>(vkView), debugName));

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
			Logger::Get().LogError(std::format(L"Doesn't support hardware downscaling of this image format: %p", m_VkImageFormat));
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

VkImageView GfxImage::GetOrCreateVkImageViewForFramebuffer(GraphicsAPI* pGraphicsAPI, uint8_t level, uint16_t layer)
{
	assert(level < sk_MaxMipLevels);
	assert(layer < sk_MaxArrayLayers);

	if (level >= sk_MaxMipLevels || layer >= sk_MaxArrayLayers)
		return VK_NULL_HANDLE;

	if (m_ImageViewForFramebuffer[level][layer] != VK_NULL_HANDLE)
		return m_ImageViewForFramebuffer[level][layer];
	

	char debugNameImageView[256]{ 0 };
	snprintf(debugNameImageView, sizeof(debugNameImageView) - 1, "Image View: '%s' imageViewForFramebuffer_[%u][%u]", m_DebugName, level, layer);

	m_ImageViewForFramebuffer[level][layer] = CreateImageView(m_pGraphicsAPI->GetGfxDevice(),
															  VK_IMAGE_VIEW_TYPE_2D,
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
