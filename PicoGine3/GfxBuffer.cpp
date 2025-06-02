#include "pch.h"
#include "GfxBuffer.h"

#if defined(_DX12)

GfxBuffer::GfxBuffer(GfxDevice* pDevice, size_t elementStride, size_t elementCount, uint32_t usageFlags, uint32_t memoryFlags, size_t minAlignmentOffset) :
	m_pGfxDevice{ pDevice },
	m_ElementStride{ elementStride },
	m_ElementCount{ elementCount },
	m_AlignmentSize{ GetAlignment(elementStride, minAlignmentOffset) },
	m_BufferSize{ elementCount * m_AlignmentSize },
	m_UsageFlags{ usageFlags },
	m_MemoryFlags{ memoryFlags }
{
	m_AlignmentSize = GetAlignment(elementStride, minAlignmentOffset);
	m_BufferSize = elementCount * m_AlignmentSize;
	//m_pGfxDevice.CreateBuffer(m_BufferSize, usageFlags, memoryFlags, m_Buffer, m_BufferMemory);
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

GfxBuffer::GfxBuffer(GfxDevice* pDevice, size_t elementStride, size_t elementCount, uint32_t usageFlags, uint32_t memoryFlags, size_t minAlignmentOffset) :
	m_pMappedMemory{ nullptr },
	m_pGfxDevice{ pDevice },
	m_ElementStride{ elementStride },
	m_ElementCount{ elementCount },
	m_UsageFlags{ usageFlags },
	m_MemoryFlags{ memoryFlags }
{
	m_AlignmentSize = GetAlignment(elementStride, minAlignmentOffset);
	m_BufferSize = elementCount * m_AlignmentSize;
	m_pGfxDevice->CreateBuffer(m_BufferSize, usageFlags, memoryFlags, m_Buffer, m_BufferMemory);
}

GfxBuffer::~GfxBuffer()
{
	const auto& device{ m_pGfxDevice->GetDevice() };

	Unmap();

	if (!m_DeferredBufferRelease)
	{
		vkDestroyBuffer(device, m_Buffer, nullptr);
		vkFreeMemory(device, m_BufferMemory, nullptr);
	}
}

void GfxBuffer::Map(size_t size, size_t offset)
{
	const size_t mappedSize{ std::min(size, m_BufferSize) };
	HandleVkResult(vkMapMemory(m_pGfxDevice->GetDevice(), m_BufferMemory, offset, mappedSize, 0, &m_pMappedMemory));
}

void GfxBuffer::Unmap()
{
	if (m_pMappedMemory)
	{
		vkUnmapMemory(m_pGfxDevice->GetDevice(), m_BufferMemory);
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
	VkMappedMemoryRange mappedRange{};
	mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedRange.memory = m_BufferMemory;
	mappedRange.offset = offset;
	mappedRange.size = size;
	HandleVkResult(vkFlushMappedMemoryRanges(m_pGfxDevice->GetDevice(), 1, &mappedRange));
}

void GfxBuffer::Invalidate(size_t size, size_t offset) const
{
	VkMappedMemoryRange mappedRange{};
	mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedRange.memory = m_BufferMemory;
	mappedRange.offset = offset;
	mappedRange.size = size;
	HandleVkResult(vkInvalidateMappedMemoryRanges(m_pGfxDevice->GetDevice(), 1, &mappedRange));
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

void GfxBuffer::DeferRelease()
{
	m_DeferredBufferRelease = true;
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
