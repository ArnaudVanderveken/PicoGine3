#ifndef GRAPHICSAPI_H
#define GRAPHICSAPI_H

#include "GfxBuffer.h"
#include "GfxCommandBuffer.h"
#include "GfxDevice.h"
#include "GfxImmediateCommands.h"
#include "GfxSwapchain.h"
#include "ShaderModulePool.h"

#if defined(_DX12)

#include <d3d12.h>
#include <dxgi1_6.h>

class GraphicsAPI
{
public:
	explicit GraphicsAPI();
	~GraphicsAPI();

	GraphicsAPI(const GraphicsAPI&) noexcept = delete;
	GraphicsAPI& operator=(const GraphicsAPI&) noexcept = delete;
	GraphicsAPI(GraphicsAPI&&) noexcept = delete;
	GraphicsAPI& operator=(GraphicsAPI&&) noexcept = delete;

	[[nodiscard]] bool IsInitialized() const;

	void BeginFrame();
	void EndFrame();
	void DrawMesh(uint32_t meshDataID, uint32_t materialID, const XMMATRIX& transform) const;

private:
	bool m_IsInitialized;

};

#elif defined(_VK)

#pragma warning(push)
#pragma warning(disable:28251)
#include <volk.h>
#pragma warning(pop)

struct PerFrameUBO
{
	alignas(16) XMFLOAT4X4 m_ViewMat;
	alignas(16) XMFLOAT4X4 m_ProjMat;
	alignas(16) XMFLOAT4X4 m_ViewInvMat;
	alignas(16) XMFLOAT4X4 m_ProjInvMat;
	alignas(16) XMFLOAT4X4 m_ViewProjMat;
	alignas(16) XMFLOAT4X4 m_ViewProjInvMat;
};

struct PushConstants
{
	alignas(16) XMFLOAT4X4 m_ModelMat;
};

struct DeferredTask
{
	explicit DeferredTask(std::packaged_task<void()>&& task, SubmitHandle handle) : m_Task(std::move(task)), m_Handle(handle) {}
	std::packaged_task<void()> m_Task;
	SubmitHandle m_Handle;
};

class GraphicsAPI final
{
public:
	explicit GraphicsAPI();
	~GraphicsAPI();

	GraphicsAPI(const GraphicsAPI&) noexcept = delete;
	GraphicsAPI& operator=(const GraphicsAPI&) noexcept = delete;
	GraphicsAPI(GraphicsAPI&&) noexcept = delete;
	GraphicsAPI& operator=(GraphicsAPI&&) noexcept = delete;

	[[nodiscard]] bool IsInitialized() const;
	[[nodiscard]] GfxDevice* GetGfxDevice() const;
	[[nodiscard]] GfxImmediateCommands* GetGfxImmediateCommands() const;
	[[nodiscard]] VkSemaphore GetTimelineSemaphore() const;
	[[nodiscard]] const GfxCommandBuffer& GetCurrentCommandBuffer() const;

	void BeginFrame();
	void EndFrame();
	void DrawMesh(uint32_t meshDataID, uint32_t materialID, const XMFLOAT4X4& transform) const;

	void AcquireCommandBuffer();
	SubmitHandle SubmitCommandBuffer(bool present = false);
	void AddDeferredTask(std::packaged_task<void()>&& task, SubmitHandle handle = SubmitHandle());

private:
	bool m_IsInitialized;

	std::unique_ptr<GfxDevice> m_pGfxDevice;
	std::unique_ptr<GfxSwapchain> m_pGfxSwapchain;
	std::unique_ptr<GfxImmediateCommands> m_pGfxImmediateCommands;
	GfxCommandBuffer m_CurrentCommandBuffer;
	VkSemaphore m_TimelineSemaphore;
	std::unique_ptr<ShaderModulePool> m_pShaderModulePool;

	std::deque<DeferredTask> m_DeferredTasks;
	
	VkDescriptorSetLayout m_VkDescriptorSetLayout;
	VkPipelineLayout m_VkGraphicsPipelineLayout;
	VkPipeline m_VkGraphicsPipeline;

	std::vector<std::unique_ptr<GfxBuffer>> m_PerFrameUBO;
	VkDescriptorPool m_VkDescriptorPool;
	std::vector<VkDescriptorSet> m_VkDescriptorSets;

	std::unique_ptr<GfxImage> m_pTestModelTextureImage;
	VkSampler m_VkTestModelTextureSampler;

	void ProcessDeferredTasks();
	void WaitDeferredTasks();

	void CreateGraphicsPipeline();
	
	void CreateDescriptorSetLayout();
	void CreateUniformBuffers();
	void UpdatePerFrameUBO() const;
	void CreateDescriptorPool();
	void CreateDescriptorSets();
	void CreateTextureImage();
};

#endif

#endif //GRAPHICSAPI_H