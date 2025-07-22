#include "pch.h"
#include "GfxCommandBuffer.h"

#include "GfxStructs.h"
#include "GfxImmediateCommands.h"
#include "GraphicsAPI.h"

GfxCommandBuffer::GfxCommandBuffer(GraphicsAPI* pGraphicsAPI) :
	m_pGraphicsAPI{ pGraphicsAPI },
	m_pWrapper{ &m_pGraphicsAPI->GetGfxImmediateCommands()->Acquire() }
{
}

GfxCommandBuffer::~GfxCommandBuffer()
{
	assert(!m_IsRendering && L"Forgot EndRendering before deleting this buffer !");
}

const CommandBufferWrapper* GfxCommandBuffer::GetBufferWrapper() const
{
	return m_pWrapper;
}

VkCommandBuffer GfxCommandBuffer::GetCmdBuffer() const
{
	return m_pWrapper ? m_pWrapper->m_CmdBuffer : VK_NULL_HANDLE;
}

void GfxCommandBuffer::TransitionToShaderReadOnly(GfxImage* texture) const
{
	assert(!texture->m_IsSwapchainImage && "Cannot transition swapchain image to shader readonly.");

	// Ignore MSAA textures
	if (texture->m_VkSamples != VK_SAMPLE_COUNT_1_BIT)
		return;

	const VkImageAspectFlags flags = texture->GetImageAspectFlags();
	// set the result of the previous render pass
	texture->TransitionLayout(m_pWrapper->m_CmdBuffer,
							  texture->IsSampledImage() ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL,
							  VkImageSubresourceRange{ flags, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS });
}

void GfxCommandBuffer::PushDebugGroupLabel(const char* label, uint32_t colorRGBA) const
{
	assert(label && "Cannot set unnamed debug event label");

	if (!label || !vkCmdBeginDebugUtilsLabelEXT)
		return;

	const VkDebugUtilsLabelEXT utilsLabel
	{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
		.pNext = nullptr,
		.pLabelName = label,
		.color = { static_cast<float>((colorRGBA >> 0) & 0xff) / 255.0f,
				  static_cast<float>((colorRGBA >> 8) & 0xff) / 255.0f,
				  static_cast<float>((colorRGBA >> 16) & 0xff) / 255.0f,
				  static_cast<float>((colorRGBA >> 24) & 0xff) / 255.0f },
	};

	vkCmdBeginDebugUtilsLabelEXT(m_pWrapper->m_CmdBuffer, &utilsLabel);
}

void GfxCommandBuffer::InsertDebugEventLabel(const char* label, uint32_t colorRGBA) const
{
	assert(label && "Cannot set unnamed debug event label");

	if (!label || !vkCmdInsertDebugUtilsLabelEXT)
		return;

	const VkDebugUtilsLabelEXT utilsLabel
	{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
		.pNext = nullptr,
		.pLabelName = label,
		.color = { static_cast<float>((colorRGBA >> 0) & 0xff) / 255.0f,
				  static_cast<float>((colorRGBA >> 8) & 0xff) / 255.0f,
				  static_cast<float>((colorRGBA >> 16) & 0xff) / 255.0f,
				  static_cast<float>((colorRGBA >> 24) & 0xff) / 255.0f },
	};

	vkCmdInsertDebugUtilsLabelEXT(m_pWrapper->m_CmdBuffer, &utilsLabel);
}

void GfxCommandBuffer::PopDebugGroupLabel() const
{
	if (!vkCmdEndDebugUtilsLabelEXT)
		return;
	
	vkCmdEndDebugUtilsLabelEXT(m_pWrapper->m_CmdBuffer);
}

void GfxCommandBuffer::BeginRendering(const RenderPass& renderPass, const Framebuffer& frameBuffer, const Dependencies& dependencies)
{
	assert(!m_IsRendering);
	m_IsRendering = true;

	for (uint32_t i{}; i < Dependencies::sk_MaxSubmitDependencies && dependencies.m_Textures[i]; ++i)
	{
		TransitionToShaderReadOnly(dependencies.m_Textures[i]);
	}

	for (uint32_t i{}; i < Dependencies::sk_MaxSubmitDependencies && dependencies.m_Buffers[i]; ++i)
	{
		VkPipelineStageFlags2 dstStageFlags = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		const VkBufferUsageFlags flags{ dependencies.m_Buffers[i]->GetBufferUsageFlags() };

		if ((flags & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) || (flags & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT))
			dstStageFlags |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;

		if (flags & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
			dstStageFlags |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;

		BufferBarrier(dependencies.m_Buffers[i], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, dstStageFlags);
	}

	const uint32_t numFbColorAttachments = frameBuffer.GetNumColorAttachments();
	const uint32_t numPassColorAttachments = renderPass.GetNumColorAttachments();

	assert(numFbColorAttachments == numPassColorAttachments);

	m_FrameBuffer = frameBuffer;

	// transition all the color attachments
	for (uint32_t i{}; i < numFbColorAttachments; ++i)
	{
		if (frameBuffer.m_Color[i].m_Texture)
		{
			TransitionToColorAttachment(frameBuffer.m_Color[i].m_Texture);
		}
		// handle MSAA
		if (frameBuffer.m_Color[i].m_ResolveTexture)
		{
			frameBuffer.m_Color[i].m_ResolveTexture->m_IsResolveAttachment = true;
			TransitionToColorAttachment(frameBuffer.m_Color[i].m_ResolveTexture);
		}
	}

	// transition depth-stencil attachment
	if (frameBuffer.m_DepthStencil.m_Texture)
	{
		assert(frameBuffer.m_DepthStencil.m_Texture->m_VkImageFormat != VK_FORMAT_UNDEFINED && L"Invalid depth attachment format");
		assert(frameBuffer.m_DepthStencil.m_Texture->m_IsDepthFormat && L"Invalid depth attachment format");
		const VkImageAspectFlags flags = frameBuffer.m_DepthStencil.m_Texture->GetImageAspectFlags();
		frameBuffer.m_DepthStencil.m_Texture->TransitionLayout(m_pWrapper->m_CmdBuffer,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VkImageSubresourceRange{ flags,
									 0,
									 VK_REMAINING_MIP_LEVELS,
									 0,
									 VK_REMAINING_ARRAY_LAYERS });
	}
	// handle depth MSAA
	if (frameBuffer.m_DepthStencil.m_ResolveTexture)
	{
		assert(frameBuffer.m_DepthStencil.m_ResolveTexture->m_IsDepthFormat && L"Invalid resolve depth attachment format");
		frameBuffer.m_DepthStencil.m_ResolveTexture->m_IsResolveAttachment = true;
		const VkImageAspectFlags flags = frameBuffer.m_DepthStencil.m_ResolveTexture->GetImageAspectFlags();
		frameBuffer.m_DepthStencil.m_ResolveTexture->TransitionLayout(m_pWrapper->m_CmdBuffer,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VkImageSubresourceRange{ flags,
									 0,
									 VK_REMAINING_MIP_LEVELS,
									 0,
									 VK_REMAINING_ARRAY_LAYERS });
	}

	VkSampleCountFlagBits samples{ VK_SAMPLE_COUNT_1_BIT };
	uint32_t mipLevel{};
	uint32_t fbWidth{};
	uint32_t fbHeight{};

	VkRenderingAttachmentInfo colorAttachments[g_MaxColorAttachment];

	for (uint32_t i{}; i != numFbColorAttachments; ++i)
	{
		const Framebuffer::AttachmentDesc& attachment{ frameBuffer.m_Color[i] };
		assert(attachment.m_Texture);

		const RenderPass::AttachmentDesc& descColor{ renderPass.m_Color[i] };
		if (mipLevel && descColor.m_Level)
			assert(descColor.m_Level == mipLevel && L"All color attachments should have the same mip-level");
		
		const VkExtent3D dim{ attachment.m_Texture->m_VkExtent };
		if (fbWidth)
			assert(dim.width == fbWidth && L"All attachments should have the same width");
		
		if (fbHeight)
			assert(dim.height == fbHeight && L"All attachments should have the same height");
		
		mipLevel = descColor.m_Level;
		fbWidth = dim.width;
		fbHeight = dim.height;
		samples = attachment.m_Texture->m_VkSamples;
		colorAttachments[i] = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.pNext = nullptr,
			.imageView = attachment.m_Texture->GetOrCreateVkImageViewForFramebuffer(descColor.m_Level, descColor.m_Layer),
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.resolveMode = (samples > 1) ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE,
			.resolveImageView = VK_NULL_HANDLE,
			.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.loadOp = LoadOpToVkAttachmentLoadOp(descColor.m_LoadOp),
			.storeOp = StoreOpToVkAttachmentStoreOp(descColor.m_StoreOp),
			.clearValue = { .color = { .float32 = { descColor.m_ClearColor[0], descColor.m_ClearColor[1], descColor.m_ClearColor[2], descColor.m_ClearColor[3] } } },
		};
		// handle MSAA
		if (descColor.m_StoreOp == StoreOp_MsaaResolve)
		{
			assert(samples > 1);
			assert(attachment.m_ResolveTexture && L"Framebuffer attachment should contain a resolve texture");

			colorAttachments[i].resolveImageView = attachment.m_ResolveTexture->GetOrCreateVkImageViewForFramebuffer(descColor.m_Level, descColor.m_Layer);
			colorAttachments[i].resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}
	}

	VkRenderingAttachmentInfo depthAttachment{};

	if (frameBuffer.m_DepthStencil.m_Texture)
	{
		const RenderPass::AttachmentDesc& descDepth{ renderPass.m_Depth };
		assert(descDepth.m_Level == mipLevel && L"Depth attachment should have the same mip-level as color attachments");

		depthAttachment = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.pNext = nullptr,
			.imageView = frameBuffer.m_DepthStencil.m_Texture->GetOrCreateVkImageViewForFramebuffer(descDepth.m_Level, descDepth.m_Layer),
			.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.resolveMode = VK_RESOLVE_MODE_NONE,
			.resolveImageView = VK_NULL_HANDLE,
			.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.loadOp = LoadOpToVkAttachmentLoadOp(descDepth.m_LoadOp),
			.storeOp = StoreOpToVkAttachmentStoreOp(descDepth.m_StoreOp),
			.clearValue = {.depthStencil = {.depth = descDepth.m_ClearDepth, .stencil = descDepth.m_ClearStencil }},
		};
		// handle depth MSAA
		if (descDepth.m_StoreOp == StoreOp_MsaaResolve)
		{
			const Framebuffer::AttachmentDesc& attachment{ frameBuffer.m_DepthStencil };
			assert(attachment.m_Texture->m_VkSamples == samples);
			assert(attachment.m_ResolveTexture && L"Framebuffer depth attachment should contain a resolve texture");

			depthAttachment.resolveImageView = attachment.m_ResolveTexture->GetOrCreateVkImageViewForFramebuffer(descDepth.m_Level, descDepth.m_Layer);
			depthAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depthAttachment.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
		}

		const VkExtent3D dim = frameBuffer.m_DepthStencil.m_Texture->m_VkExtent;
		if (fbWidth)
			assert(dim.width == fbWidth && L"All attachments should have the same width");
		
		if (fbHeight)
			assert(dim.height == fbHeight && L"All attachments should have the same height");
		
		mipLevel = descDepth.m_Level;
		fbWidth = dim.width;
		fbHeight = dim.height;

		const uint32_t width{ std::max(fbWidth >> mipLevel, 1u) };
		const uint32_t height{ std::max(fbHeight >> mipLevel, 1u) };
		const Viewport viewport{ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
		const ScissorRect scissor{ 0, 0, width, height };

		VkRenderingAttachmentInfo stencilAttachment{ depthAttachment };

		const bool isStencilFormat{ renderPass.m_Stencil.m_LoadOp != LoadOp_Invalid };

		const VkRenderingInfo renderingInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.pNext = nullptr,
			.flags = 0,
			.renderArea = { VkOffset2D{ static_cast<int32_t>(scissor.m_X), static_cast<int32_t>(scissor.m_Y) }, VkExtent2D{ scissor.m_Width, scissor.m_Height }},
			.layerCount = 1,
			.viewMask = 0,
			.colorAttachmentCount = numFbColorAttachments,
			.pColorAttachments = colorAttachments,
			.pDepthAttachment = frameBuffer.m_DepthStencil.m_Texture ? &depthAttachment : nullptr,
			.pStencilAttachment = isStencilFormat ? &stencilAttachment : nullptr,
		};

		BindViewport(viewport);
		BindScissorRect(scissor);
		BindDepthState({});

		m_pGraphicsAPI->CheckAndUpdateDescriptorSets();

		vkCmdSetDepthCompareOp(m_pWrapper->m_CmdBuffer, VK_COMPARE_OP_ALWAYS);
		vkCmdSetDepthBiasEnable(m_pWrapper->m_CmdBuffer, VK_FALSE);

		vkCmdBeginRendering(m_pWrapper->m_CmdBuffer, &renderingInfo);
	}
}

void GfxCommandBuffer::EndRendering()
{
	m_IsRendering = false;

	vkCmdEndRendering(m_pWrapper->m_CmdBuffer);

	m_FrameBuffer = {};
}

void GfxCommandBuffer::BindViewport(const Viewport& viewport) const
{
	// https://www.saschawillems.de/blog/2019/03/29/flipping-the-vulkan-viewport/
	const VkViewport vp = {
		.x = viewport.m_X, // float x;
		.y = viewport.m_Height - viewport.m_Y, // float y;
		.width = viewport.m_Width, // float width;
		.height = -viewport.m_Height, // float height;
		.minDepth = viewport.m_MinDepth, // float minDepth;
		.maxDepth = viewport.m_MaxDepth, // float maxDepth;
	};
	vkCmdSetViewport(m_pWrapper->m_CmdBuffer, 0, 1, &vp);
}

void GfxCommandBuffer::BindScissorRect(const ScissorRect& scissor) const
{
	const VkRect2D scissorRect
	{
	  VkOffset2D{ static_cast<int32_t>(scissor.m_X), static_cast<int32_t>(scissor.m_Y) },
	  VkExtent2D{ scissor.m_Width, scissor.m_Height },
	};
	vkCmdSetScissor(m_pWrapper->m_CmdBuffer, 0, 1, &scissorRect);
}

void GfxCommandBuffer::BindRenderPipeline(const GfxRenderPipeline& pipeline)
{
}

void GfxCommandBuffer::BindDepthState(const DepthState& state)
{
}

void GfxCommandBuffer::BindVertexBuffer(uint32_t index, GfxBuffer* buffer, uint64_t bufferOffset)
{
}

void GfxCommandBuffer::BindIndexBuffer(GfxBuffer* buffer, IndexFormat indexFormat, uint64_t indexBufferOffset)
{
}

void GfxCommandBuffer::BindPushConstants(const void* data, size_t size, size_t offset)
{
}

void GfxCommandBuffer::FillBuffer(GfxBuffer* buffer, size_t bufferOffset, size_t size, uint32_t data)
{
}

void GfxCommandBuffer::UpdateBuffer(GfxBuffer* buffer, size_t bufferOffset, size_t size, const void* data)
{
}

void GfxCommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t baseInstance)
{
}

void GfxCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
	int32_t vertexOffset, uint32_t baseInstance)
{
}

void GfxCommandBuffer::DrawIndirect(GfxBuffer* indirectBuffer, size_t indirectBufferOffset, uint32_t drawCount,
	uint32_t stride)
{
}

void GfxCommandBuffer::DrawIndexedIndirect(GfxBuffer* indirectBuffer, size_t indirectBufferOffset, uint32_t drawCount,
	uint32_t stride)
{
}

void GfxCommandBuffer::DrawIndexedIndirectCount(GfxBuffer* indirectBuffer, size_t indirectBufferOffset,
	GfxBuffer* countBuffer, size_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
}

void GfxCommandBuffer::ClearColorImage(GfxImage* tex, const ClearColorValue& value, const TextureLayers& layers)
{
}

void GfxCommandBuffer::CopyImage(GfxImage* src, GfxImage* dst, const Dimensions& extent, const Offset3D& srcOffset,
	const Offset3D& dstOffset, const TextureLayers& srcLayers, const TextureLayers& dstLayers)
{
}

void GfxCommandBuffer::GenerateMipmap(GfxImage* texture)
{
}

void GfxCommandBuffer::UseComputeTexture(GfxImage* texture, VkPipelineStageFlags2 dstStage) const
{
	if (!texture->IsStorageImage())
	{
		Logger::Get().LogWarning(L"Calling UseComputeTexture on an image without the TextureUsageBits::Storage");
		return;
	}

	texture->TransitionLayout(m_pWrapper->m_CmdBuffer,
							  VK_IMAGE_LAYOUT_GENERAL,
							  VkImageSubresourceRange{ texture->GetImageAspectFlags(), 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS });
}

void GfxCommandBuffer::BufferBarrier(GfxBuffer* buffer, VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage)
{
	VkBufferMemoryBarrier2 barrier
	{
	  .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
	  .srcStageMask = srcStage,
	  .srcAccessMask = 0,
	  .dstStageMask = dstStage,
	  .dstAccessMask = 0,
	  .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	  .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	  .buffer = buffer->GetBuffer(),
	  .offset = 0,
	  .size = VK_WHOLE_SIZE,
	};

	if (srcStage & VK_PIPELINE_STAGE_2_TRANSFER_BIT)
		barrier.srcAccessMask |= VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT;
	else
		barrier.srcAccessMask |= VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;

	if (dstStage & VK_PIPELINE_STAGE_2_TRANSFER_BIT)
		barrier.dstAccessMask |= VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT;
	else
		barrier.dstAccessMask |= VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
	
	if (dstStage & VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT)
		barrier.dstAccessMask |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
	
	if (buffer->GetBufferUsageFlags() & VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
		barrier.dstAccessMask |= VK_ACCESS_2_INDEX_READ_BIT;

	const VkDependencyInfo depInfo
	{
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.bufferMemoryBarrierCount = 1,
		.pBufferMemoryBarriers = &barrier,
	};

	vkCmdPipelineBarrier2(m_pWrapper->m_CmdBuffer, &depInfo);
}

void GfxCommandBuffer::TransitionToColorAttachment(GfxImage* texture) const
{
	assert(texture->m_IsDepthFormat && L"Color attachments cannot have depth format");
	assert(texture->m_IsStencilFormat && L"Color attachments cannot have stencil format");
	assert(texture->m_VkImageFormat != VK_FORMAT_UNDEFINED && L"Invalid color attachment format");

	texture->TransitionLayout(m_pWrapper->m_CmdBuffer,
							  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
							  VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS });
}
