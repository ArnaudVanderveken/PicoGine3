#include "pch.h"
#include "GfxImmediateCommands.h"

#include "GfxDevice.h"

SubmitHandle::SubmitHandle(uint64_t handle) :
	m_BufferIndex(static_cast<uint32_t>(handle & 0xffffffff)),
	m_SubmitId(static_cast<uint32_t>(handle >> 32))
{
}

bool SubmitHandle::Empty() const
{
	return m_SubmitId == 0;
}

uint64_t SubmitHandle::Handle() const
{
	return (static_cast<uint64_t>(m_SubmitId) << 32) + m_BufferIndex;
}

GfxImmediateCommands::GfxImmediateCommands(GfxDevice* pDevice, const char* debugName) :
	m_pDevice{ pDevice },
	m_DebugName{ debugName }
{
	const auto& queueInfo{ m_pDevice->GetDeviceQueueInfo() };
	m_QueueFamilyIndex = queueInfo.m_GraphicsFamily;
	m_Queue = queueInfo.m_GraphicsQueue;

	const auto& device{ m_pDevice->GetDevice() };

	const VkCommandPoolCreateInfo poolCreateInfo
	{
	  .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
	  .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
	  .queueFamilyIndex = m_QueueFamilyIndex,
	};
	HandleVkResult(vkCreateCommandPool(device, &poolCreateInfo, nullptr, &m_CommandPool));
	HandleVkResult(m_pDevice->SetVkObjectName(VK_OBJECT_TYPE_COMMAND_POOL, reinterpret_cast<uint64_t>(m_CommandPool), debugName));

	const VkCommandBufferAllocateInfo bufferAllocInfo
	{
	  .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
	  .commandPool = m_CommandPool,
	  .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	  .commandBufferCount = 1,
	};

	for (uint32_t i{}; i < sk_MaxCommandBuffers; ++i)
	{
		CommandBufferWrapper& buf{ m_CmdBuffers[i] };
		char fenceName[256]{ 0 };
		char semaphoreName[256]{ 0 };
		if (debugName)
		{
			snprintf(fenceName, sizeof(fenceName) - 1, "Fence: %s (CmdBuf %u)", debugName, i);
			snprintf(semaphoreName, sizeof(semaphoreName) - 1, "Semaphore: %s (CmdBuf %u)", debugName, i);
		}
		buf.m_Semaphore = m_pDevice->CreateVkSemaphore(semaphoreName);
		buf.m_Fence = m_pDevice->CreateVkFence(fenceName);
		HandleVkResult(vkAllocateCommandBuffers(device, &bufferAllocInfo, &buf.m_CmdBufferAllocated));
		m_CmdBuffers[i].m_Handle.m_BufferIndex = i;
	}
}

GfxImmediateCommands::~GfxImmediateCommands()
{
	WaitAll();

	const auto& device{ m_pDevice->GetDevice() };

	for (const CommandBufferWrapper& buf : m_CmdBuffers)
	{
		// lifetimes of all VkFence objects are managed explicitly we do not use deferredTask() for them
		vkDestroyFence(device, buf.m_Fence, nullptr);
		vkDestroySemaphore(device, buf.m_Semaphore, nullptr);
	}

	vkDestroyCommandPool(device, m_CommandPool, nullptr);
}

const CommandBufferWrapper& GfxImmediateCommands::Acquire()
{
	auto& logger{ Logger::Get() };

	if (!m_NumAvailableCommandBuffers)
		Purge();

	while (!m_NumAvailableCommandBuffers)
	{
		logger.LogInfo(L"Waiting for command buffers...\n");
		Purge();
	}

	CommandBufferWrapper* current{};

	// we are ok with any available buffer
	for (CommandBufferWrapper& buffer : m_CmdBuffers)
	{
		if (buffer.m_CmdBuffer == VK_NULL_HANDLE)
		{
			current = &buffer;
			break;
		}
	}

	current->m_Handle.m_SubmitId = m_SubmitCounter;
	--m_NumAvailableCommandBuffers;

	current->m_CmdBuffer = current->m_CmdBufferAllocated;
	current->m_IsEncoding = true;

	constexpr VkCommandBufferBeginInfo beginInfo
	{
	  .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	  .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	HandleVkResult(vkBeginCommandBuffer(current->m_CmdBuffer, &beginInfo));

	m_NextSubmitHandle = current->m_Handle;

	return *current;
}

SubmitHandle GfxImmediateCommands::Submit(const CommandBufferWrapper& buffer)
{
	HandleVkResult(vkEndCommandBuffer(buffer.m_CmdBuffer));

	VkSemaphoreSubmitInfo waitSemaphores[]{ {}, {} };
	uint32_t numWaitSemaphores{};
	if (m_WaitSemaphore.semaphore)
		waitSemaphores[numWaitSemaphores++] = m_WaitSemaphore;
	
	if (m_LastSubmitSemaphore.semaphore)
		waitSemaphores[numWaitSemaphores] = m_LastSubmitSemaphore;
	
	VkSemaphoreSubmitInfo signalSemaphores[]
	{
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = buffer.m_Semaphore,
			.stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
		},
		{},
	};
	uint32_t numSignalSemaphores{ 1 };
	if (m_SignalSemaphore.semaphore)
		signalSemaphores[numSignalSemaphores++] = m_SignalSemaphore;

	const VkCommandBufferSubmitInfo bufferSubmitInfo
	{
	  .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
	  .commandBuffer = buffer.m_CmdBuffer,
	};

	const VkSubmitInfo2 submitInfo
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.waitSemaphoreInfoCount = numWaitSemaphores,
		.pWaitSemaphoreInfos = waitSemaphores,
		.commandBufferInfoCount = 1u,
		.pCommandBufferInfos = &bufferSubmitInfo,
		.signalSemaphoreInfoCount = numSignalSemaphores,
		.pSignalSemaphoreInfos = signalSemaphores,
	};

	HandleVkResult(vkQueueSubmit2(m_Queue, 1u, &submitInfo, buffer.m_Fence));

	m_LastSubmitSemaphore.semaphore = buffer.m_Semaphore;
	m_LastSubmitHandle = buffer.m_Handle;
	m_WaitSemaphore.semaphore = VK_NULL_HANDLE;
	m_SignalSemaphore.semaphore = VK_NULL_HANDLE;

	const_cast<CommandBufferWrapper&>(buffer).m_IsEncoding = false;
	++m_SubmitCounter;

	// skip the 0 value when uint32_t wraps around (null SubmitHandle)
	if (!m_SubmitCounter)
		++m_SubmitCounter;

	return m_LastSubmitHandle;
}

void GfxImmediateCommands::WaitSemaphore(VkSemaphore semaphore)
{
	m_WaitSemaphore.semaphore = semaphore;
}

void GfxImmediateCommands::SignalSemaphore(VkSemaphore semaphore, uint64_t signalValue)
{
	m_SignalSemaphore.semaphore = semaphore;
	m_SignalSemaphore.value = signalValue;
}

VkSemaphore GfxImmediateCommands::AcquireLastSubmitSemaphore()
{
	return std::exchange(m_LastSubmitSemaphore.semaphore, VK_NULL_HANDLE);
}

VkFence GfxImmediateCommands::GetVkFence(SubmitHandle handle) const
{
	if (handle.Empty())
		return VK_NULL_HANDLE;

	return m_CmdBuffers[handle.m_BufferIndex].m_Fence;
}

SubmitHandle GfxImmediateCommands::GetLastSubmitHandle() const
{
	return m_LastSubmitHandle;
}

SubmitHandle GfxImmediateCommands::GetNextSubmitHandle() const
{
	return m_NextSubmitHandle;
}

bool GfxImmediateCommands::IsReady(SubmitHandle handle, bool fastCheckNoVulkan) const
{
	// Empty handle
	if (handle.Empty())
		return true;

	const CommandBufferWrapper& buffer = m_CmdBuffers[handle.m_BufferIndex];

	// Already recycled
	if (buffer.m_CmdBuffer == VK_NULL_HANDLE)
		return true;

	// Already recycled and reused
	if (buffer.m_Handle.m_SubmitId != handle.m_SubmitId)
		return true;
	

	if (fastCheckNoVulkan)
		return false; // Skip Vulkan API query, just let it retire naturally (when submitId for this bufferIndex gets incremented)

	const auto& device{ m_pDevice->GetDevice() };
	return vkWaitForFences(device, 1, &buffer.m_Fence, VK_TRUE, 0) == VK_SUCCESS;
}

void GfxImmediateCommands::Wait(SubmitHandle handle)
{
	const auto& device{ m_pDevice->GetDevice() };
	auto& logger{ Logger::Get() };
	if (handle.Empty())
	{
		vkDeviceWaitIdle(device);
		return;
	}

	if (IsReady(handle))
			return;

	if (!m_CmdBuffers[handle.m_BufferIndex].m_IsEncoding)
	{
		// we are waiting for a buffer which has not been submitted - this is probably a logic error somewhere in the calling code
		logger.LogWarning(L"Waiting on a command buffer that hasn't been submitted yet. Potential code logic issue.");
		return;
	}

	HandleVkResult(vkWaitForFences(device, 1, &m_CmdBuffers[handle.m_BufferIndex].m_Fence, VK_TRUE, UINT64_MAX));

	Purge();
}

void GfxImmediateCommands::WaitAll()
{
	VkFence fences[sk_MaxCommandBuffers];

	uint32_t numFences{};

	for (const CommandBufferWrapper& buffer : m_CmdBuffers)
		if (buffer.m_CmdBuffer != VK_NULL_HANDLE && !buffer.m_IsEncoding)
			fences[numFences++] = buffer.m_Fence;

	if (numFences)
	{
		const auto& device{ m_pDevice->GetDevice() };
		HandleVkResult(vkWaitForFences(device, numFences, fences, VK_TRUE, UINT64_MAX));
	}

	Purge();
}

void GfxImmediateCommands::Purge()
{
	const auto& device{ m_pDevice->GetDevice() };

	for (uint32_t i{}; i < sk_MaxCommandBuffers; ++i)
	{
		// always start checking with the oldest submitted buffer, then wrap around
		CommandBufferWrapper& buffer{ m_CmdBuffers[(i + m_LastSubmitHandle.m_BufferIndex + 1) % sk_MaxCommandBuffers] };

		if (buffer.m_CmdBuffer == VK_NULL_HANDLE || buffer.m_IsEncoding)
			continue;

		const VkResult result{ vkWaitForFences(device, 1, &buffer.m_Fence, VK_TRUE, 0) };

		if (result == VK_SUCCESS)
		{
			HandleVkResult(vkResetCommandBuffer(buffer.m_CmdBuffer, VkCommandBufferResetFlags{ 0 }));
			HandleVkResult(vkResetFences(device, 1, &buffer.m_Fence));
			buffer.m_CmdBuffer = VK_NULL_HANDLE;
			++m_NumAvailableCommandBuffers;
		}
		else
		{
			if (result != VK_TIMEOUT)
				HandleVkResult(result);
		}
	}
}
