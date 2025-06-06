#ifndef GFXBUFFER_H
#define GFXBUFFER_H


class GraphicsAPI;

class GfxBuffer final
{
public:
	explicit GfxBuffer(GraphicsAPI* pGraphicsAPI, size_t elementStride, size_t elementCount, uint32_t usageFlags, uint32_t memoryFlags, size_t minAlignmentOffset = 0);
	~GfxBuffer();

	GfxBuffer(const GfxBuffer& other) noexcept = delete;
	GfxBuffer& operator=(const GfxBuffer& other) noexcept = delete;
	GfxBuffer(GfxBuffer&& other) noexcept = delete;
	GfxBuffer& operator=(GfxBuffer&& other) noexcept = delete;

	void Map(size_t size = ~0ULL, size_t offset = 0);
	void Unmap();

	[[nodiscard]] size_t GetBufferSize() const;

	void WriteToBuffer(const void* data, size_t size = ~0ULL, size_t offset = 0) const;
	void Flush(size_t size = ~0ULL, size_t offset = 0) const;
	void Invalidate(size_t size = ~0ULL, size_t offset = 0) const;

	void WriteToIndex(const void* data, int index) const;
	void FlushIndex(int index) const;
	void InvalidateIndex(int index) const;

#if defined(_DX12)
	[[nodiscard]] void* GetBuffer() const;
#elif defined(_VK)
	[[nodiscard]] VkBuffer GetBuffer() const;
	[[nodiscard]] VkDeviceMemory GetBufferMemory() const;
#endif

private:
#if defined(_DX12)

#elif defined(_VK)

	VkBuffer m_Buffer;
	VkDeviceMemory m_BufferMemory;
	void* m_pMappedMemory;

#endif

	GraphicsAPI* m_pGraphicsAPI;

	size_t m_ElementStride;
	size_t m_ElementCount;
	size_t m_AlignmentSize;
	size_t m_BufferSize;

	uint32_t m_UsageFlags;
	uint32_t m_MemoryFlags;

	static size_t GetAlignment(size_t elementStride, size_t minAlignmentOffset);
};

#endif //GFXBUFFER_H