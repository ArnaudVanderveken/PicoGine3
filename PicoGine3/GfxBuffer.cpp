#include "pch.h"
#include "GfxBuffer.h"

#if defined(_DX12)

GfxBuffer::GfxBuffer(const GfxDevice& device, size_t elementStride, size_t elementCount, uint32_t usageFlags, uint32_t memoryFlags, size_t minAlignmentOffset) :
	m_GfxDevice{ device },
	m_ElementStride{ elementStride },
	m_ElementCount{ elementCount },
	m_AlignmentSize{ GetAlignment(elementStride, minAlignmentOffset) },
	m_BufferSize{ elementCount * m_AlignmentSize },
	m_UsageFlags{ usageFlags },
	m_MemoryFlags{ memoryFlags }
{
	m_AlignmentSize = GetAlignment(elementStride, minAlignmentOffset);
	m_BufferSize = elementCount * m_AlignmentSize;
	//m_GfxDevice.CreateBuffer(m_BufferSize, usageFlags, memoryFlags, m_Buffer, m_BufferMemory);
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

GfxBuffer::GfxBuffer(const GfxDevice& device, size_t elementStride, size_t elementCount, uint32_t usageFlags, uint32_t memoryFlags, size_t minAlignmentOffset) :
	m_GfxDevice{ device },
	m_ElementStride{ elementStride },
	m_ElementCount{ elementCount },
	m_UsageFlags{ usageFlags },
	m_MemoryFlags{ memoryFlags }
{
	m_AlignmentSize = GetAlignment(elementStride, minAlignmentOffset);
	m_BufferSize = elementCount * m_AlignmentSize;
	m_GfxDevice.CreateBuffer(m_BufferSize, usageFlags, memoryFlags, m_Buffer, m_BufferMemory);
}

GfxBuffer::~GfxBuffer()
{
	const auto& device{ m_GfxDevice.GetDevice() };

	Unmap();
	vkDestroyBuffer(device, m_Buffer, nullptr);
	vkFreeMemory(device, m_BufferMemory, nullptr);
}

void GfxBuffer::Map(size_t size, size_t offset)
{
	HandleVkResult(vkMapMemory(m_GfxDevice.GetDevice(), m_BufferMemory, offset, size, 0, &m_pMappedMemory));
}

void GfxBuffer::Unmap()
{
	if (m_pMappedMemory)
	{
		vkUnmapMemory(m_GfxDevice.GetDevice(), m_BufferMemory);
		m_pMappedMemory = nullptr;
	}
}

VkBuffer GfxBuffer::GetBuffer() const
{
	return m_Buffer;
}

#endif

size_t GfxBuffer::GetAlignment(size_t elementStride, size_t minAlignmentOffset)
{
	if (minAlignmentOffset > 0)
		return (elementStride + minAlignmentOffset - 1) & ~(minAlignmentOffset - 1);

	return elementStride;
}
