#ifndef RESOURCEDATA_H
#define RESOURCEDATA_H


struct MeshData
{
#if defined(_VK)
	VkBuffer m_VertexBuffer;
	VkDeviceMemory m_VertexBufferMemory;
	VkBuffer m_IndexBuffer;
	VkDeviceMemory m_IndexBufferMemory;
#elif defined(_DX)

#endif

	uint32_t m_IndexCount{};
};

//struct TextureData
//{
//#if defined(_VK)
//	VkImage m_GPUImage{};
//	VkDeviceMemory m_GPUMemory{};
//	VkImageView m_GPUImageView{};
//#elif defined (_DX)
//
//#endif
//
//	uint32_t m_Width{}, m_Height{}, m_Channels{};
//	
//};


#endif //RESOURCEDATA_H