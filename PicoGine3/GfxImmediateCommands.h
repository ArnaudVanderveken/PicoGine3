#ifndef GFXIMMEDIATECOMMANDS_H
#define GFXIMMEDIATECOMMANDS_H

struct SubmitHandle
{
	uint32_t m_BufferIndex{};
	uint32_t m_SubmitId{};
	SubmitHandle() = default;
	explicit SubmitHandle(uint64_t handle);
	[[nodiscard]] bool Empty() const;
	[[nodiscard]] uint64_t Handle() const;
};

struct CommandBufferWrapper
{
	VkCommandBuffer m_CmdBuffer{ VK_NULL_HANDLE };
	VkCommandBuffer m_CmdBufferAllocated{ VK_NULL_HANDLE };
	SubmitHandle m_Handle{};
	VkFence m_Fence{ VK_NULL_HANDLE };
	VkSemaphore m_Semaphore{ VK_NULL_HANDLE };
	bool m_IsEncoding{};
};

class GfxDevice;

class GfxImmediateCommands final
{
public:
	explicit GfxImmediateCommands(GfxDevice* pDevice, const char* debugName = nullptr);
	~GfxImmediateCommands();

	GfxImmediateCommands(const GfxImmediateCommands&) noexcept = delete;
	GfxImmediateCommands& operator=(const GfxImmediateCommands&) noexcept = delete;
	GfxImmediateCommands(GfxImmediateCommands&&) noexcept = delete;
	GfxImmediateCommands& operator=(GfxImmediateCommands&&) noexcept = delete;

	static constexpr uint32_t sk_MaxCommandBuffers{ 64 };

	const CommandBufferWrapper& Acquire();
	SubmitHandle Submit(const CommandBufferWrapper& buffer);
	void WaitSemaphore(VkSemaphore semaphore);
	void SignalSemaphore(VkSemaphore semaphore, uint64_t signalValue);
	[[nodiscard]] VkSemaphore AcquireLastSubmitSemaphore();
	[[nodiscard]] VkFence GetVkFence(SubmitHandle handle) const;
	[[nodiscard]] SubmitHandle GetLastSubmitHandle() const;
	[[nodiscard]] SubmitHandle GetNextSubmitHandle() const;
	[[nodiscard]] bool IsReady(SubmitHandle handle, bool fastCheckNoVulkan = false) const;
	void Wait(SubmitHandle handle);
	void WaitAll();

private:
	GfxDevice* m_pDevice{};
	VkQueue m_Queue{ VK_NULL_HANDLE };
	VkCommandPool m_CommandPool{ VK_NULL_HANDLE };
	uint32_t m_QueueFamilyIndex{};
	const char* m_DebugName{};
	CommandBufferWrapper m_CmdBuffers[sk_MaxCommandBuffers];
	SubmitHandle m_LastSubmitHandle{};
	SubmitHandle m_NextSubmitHandle{};
	VkSemaphoreSubmitInfo m_LastSubmitSemaphore
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
	};
	VkSemaphoreSubmitInfo m_WaitSemaphore
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
	};
	VkSemaphoreSubmitInfo m_SignalSemaphore
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
	};
	uint32_t m_NumAvailableCommandBuffers{ sk_MaxCommandBuffers };
	uint32_t m_SubmitCounter{ 1 };
	
	void Purge();
};

#endif //GFXIMMEDIATECOMMANDS_H