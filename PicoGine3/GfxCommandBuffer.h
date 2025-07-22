#ifndef GFXCOMMANDBUFFER_H
#define GFXCOMMANDBUFFER_H

#include "GfxRenderPipeline.h"

#include "GfxStructs.h"
class GraphicsAPI;
struct CommandBufferWrapper;

class GfxCommandBuffer final
{
public:
	explicit GfxCommandBuffer() = default;
	explicit GfxCommandBuffer(GraphicsAPI* pGraphicsAPI);
	~GfxCommandBuffer();

	GfxCommandBuffer(const GfxCommandBuffer&) noexcept = delete;
	GfxCommandBuffer& operator=(const GfxCommandBuffer&) noexcept = delete;
	GfxCommandBuffer(GfxCommandBuffer&&) noexcept = default;
	GfxCommandBuffer& operator=(GfxCommandBuffer&&) noexcept = default;

	[[nodiscard]] const CommandBufferWrapper* GetBufferWrapper() const;
	[[nodiscard]] VkCommandBuffer GetCmdBuffer() const;

	void TransitionToShaderReadOnly(GfxImage* texture) const;

	void PushDebugGroupLabel(const char* label, uint32_t colorRGBA = 0xFFFFFFFF) const;
	void InsertDebugEventLabel(const char* label, uint32_t colorRGBA = 0xFFFFFFFF) const;
	void PopDebugGroupLabel() const;
	

	//void BindRayTracingPipeline(const GfxRayTracingPipeline& pipeline);

	//void BindComputePipeline(const GfxComputePipeline& pipeline);
	//void DispatchThreadGroups(const ThreadGroupDimensions& dimensions, const Dependencies& dependencies = {});

	void BeginRendering(const RenderPass& renderPass, const Framebuffer& frameBuffer, const Dependencies& dependencies = {});
	void EndRendering();

	void BindViewport(const Viewport& viewport) const;
	void BindScissorRect(const ScissorRect& scissor) const;

	void BindRenderPipeline(const GfxRenderPipeline& pipeline);
	void BindDepthState(const DepthState& state);

	void BindVertexBuffer(uint32_t index, GfxBuffer* buffer, uint64_t bufferOffset = 0);
	void BindIndexBuffer(GfxBuffer* buffer, IndexFormat indexFormat, uint64_t indexBufferOffset = 0);
	void BindPushConstants(const void* data, size_t size, size_t offset = 0);
	template<typename Struct>
	void PushConstants(const Struct& data, size_t offset = 0)
	{
		this->PushConstants(&data, sizeof(Struct), offset);
	}

	void FillBuffer(GfxBuffer* buffer, size_t bufferOffset, size_t size, uint32_t data);
	void UpdateBuffer(GfxBuffer* buffer, size_t bufferOffset, size_t size, const void* data);
	template<typename Struct>
	void UpdateBuffer(GfxBuffer* buffer, const Struct& data, size_t bufferOffset = 0)
	{
		this->UpdateBuffer(buffer, bufferOffset, sizeof(Struct), &data);
	}

	void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t baseInstance = 0);
	void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t baseInstance = 0);
	void DrawIndirect(GfxBuffer* indirectBuffer, size_t indirectBufferOffset, uint32_t drawCount, uint32_t stride = 0);
	void DrawIndexedIndirect(GfxBuffer* indirectBuffer, size_t indirectBufferOffset, uint32_t drawCount, uint32_t stride = 0);
	void DrawIndexedIndirectCount(GfxBuffer* indirectBuffer, size_t indirectBufferOffset, GfxBuffer* countBuffer, size_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride = 0);

	//void DrawMeshTasks(const Dimensions& threadGroupCount);
	//void DrawMeshTasksIndirect(GfxBuffer* indirectBuffer, size_t indirectBufferOffset, uint32_t drawCount, uint32_t stride = 0);
	//void DrawMeshTasksIndirectCount(GfxBuffer* indirectBuffer, size_t indirectBufferOffset, GfxBuffer* countBuffer, size_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride = 0);

	//void TraceRays(uint32_t width, uint32_t height, uint32_t depth, const Dependencies& dependencies);

	//void SetBlendColor(const float color[4]);
	//void SetDepthBias(float constantFactor, float slopeFactor, float clamp = 0.0f);
	//void SetDepthBiasEnable(bool enable);

	//void ResetQueryPool(QueryPoolHandle pool, uint32_t firstQuery, uint32_t queryCount);
	//void WriteTimestamp(QueryPoolHandle pool, uint32_t query);

	void ClearColorImage(GfxImage* tex, const ClearColorValue& value, const TextureLayers& layers = {});
	void CopyImage(GfxImage* src, GfxImage* dst, const Dimensions& extent, const Offset3D& srcOffset = {}, const Offset3D& dstOffset = {}, const TextureLayers& srcLayers = {}, const TextureLayers& dstLayers = {});
	void GenerateMipmap(GfxImage* texture);
	//void UpdateTLAS(AccelStructHandle handle, BufferHandle instancesBuffer) override;

private:
	GraphicsAPI* m_pGraphicsAPI{};
	const CommandBufferWrapper* m_pWrapper{};

	bool m_IsRendering{};
	Framebuffer m_FrameBuffer{};

	void UseComputeTexture(GfxImage* texture, VkPipelineStageFlags2 dstStage) const;
	void BufferBarrier(GfxBuffer* buffer, VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage);
	void TransitionToColorAttachment(GfxImage* texture) const;
	
};

#endif //GFXCOMMANDBUFFER_H