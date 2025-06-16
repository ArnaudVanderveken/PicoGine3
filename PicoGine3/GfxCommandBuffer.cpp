#include "pch.h"
#include "GfxCommandBuffer.h"

#include "GfxStructs.h"
#include "GfxImmediateCommands.h"

GfxCommandBuffer::GfxCommandBuffer(GfxImmediateCommands* pGfxImmediateCommands) :
	m_pGfxImmediateCommands{ pGfxImmediateCommands },
	m_pWrapper{ &m_pGfxImmediateCommands->Acquire() }
{
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
}

void GfxCommandBuffer::EndRendering()
{
}

void GfxCommandBuffer::BindViewport(const Viewport& viewport)
{
}

void GfxCommandBuffer::BindScissorRect(const ScissorRect& scissor)
{
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

void GfxCommandBuffer::UseComputeTexture(GfxImage* texture, VkPipelineStageFlags2 dstStage)
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
	
	if (buf->vkUsageFlags_ & VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
		barrier.dstAccessMask |= VK_ACCESS_2_INDEX_READ_BIT;

	const VkDependencyInfo depInfo
	{
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.bufferMemoryBarrierCount = 1,
		.pBufferMemoryBarriers = &barrier,
	};

	vkCmdPipelineBarrier2(m_pWrapper->m_CmdBuffer, &depInfo);
}
