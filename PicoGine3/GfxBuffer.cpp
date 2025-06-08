#include "pch.h"
#include "GfxBuffer.h"

#include "GraphicsAPI.h"

#if defined(_DX12)

GfxBuffer::GfxBuffer(GfxDevice* pDevice, size_t elementStride, size_t elementCount, uint32_t usageFlags, uint32_t memoryFlags, size_t minAlignmentOffset) :
	m_pGraphicsAPI{ pDevice },
	m_ElementStride{ elementStride },
	m_ElementCount{ elementCount },
	m_AlignmentSize{ GetAlignment(elementStride, minAlignmentOffset) },
	m_BufferSize{ elementCount * m_AlignmentSize },
	m_UsageFlags{ usageFlags },
	m_MemoryFlags{ memoryFlags }
{
	m_AlignmentSize = GetAlignment(elementStride, minAlignmentOffset);
	m_BufferSize = elementCount * m_AlignmentSize;
	//m_pGraphicsAPI.CreateBuffer(m_BufferSize, usageFlags, memoryFlags, m_Buffer, m_BufferMemory);
}

GfxBuffer::~GfxBuffer()
{
}

GfxBuffer::~GfxBuffer()
{
}

void GfxBuffer::Map(size_t size, size_t offset)
{
}

void GfxBuffer::Unmap()
{
}

void* GetBuffer()
{
	return nullptr;
}

#elif defined(_VK)

GfxBuffer::GfxBuffer(GraphicsAPI* pGraphicsAPI, size_t elementStride, size_t elementCount, uint32_t usageFlags, uint32_t memoryFlags, size_t minAlignmentOffset) :
	m_pMappedMemory{ nullptr },
	m_pGraphicsAPI{ pGraphicsAPI},
	m_ElementStride{ elementStride },
	m_ElementCount{ elementCount },
	m_UsageFlags{ usageFlags },
	m_MemoryFlags{ memoryFlags }
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

#endif

size_t GfxBuffer::GetAlignment(size_t elementStride, size_t minAlignmentOffset)
{
	if (minAlignmentOffset > 0)
		return (elementStride + minAlignmentOffset - 1) & ~(minAlignmentOffset - 1);

	return elementStride;
}
