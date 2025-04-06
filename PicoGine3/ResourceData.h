#ifndef RESOURCEDATA_H
#define RESOURCEDATA_H

#include "GfxBuffer.h"
#include "Vertex.h"

struct MeshData
{
	explicit MeshData(const GfxDevice& device, const std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& indices) :
		m_VertexBuffer{ device, sizeof(Vertex3D), vertices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
		m_IndexBuffer{ device, sizeof(uint32_t), indices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
		m_IndexCount{ static_cast<uint32_t>(indices.size()) }
	{
		GfxBuffer stagingVertexBuffer{ device, sizeof(Vertex3D), vertices.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

		stagingVertexBuffer.Map();
		stagingVertexBuffer.WriteToBuffer(vertices.data());
		stagingVertexBuffer.Unmap();

		GfxBuffer stagingIndexBuffer{ device, sizeof(uint32_t), indices.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

		stagingIndexBuffer.Map();
		stagingIndexBuffer.WriteToBuffer(indices.data());
		stagingIndexBuffer.Unmap();

		device.CopyBuffer(stagingVertexBuffer.GetBuffer(), m_VertexBuffer.GetBuffer(), stagingVertexBuffer.GetBufferSize());
		device.CopyBuffer(stagingIndexBuffer.GetBuffer(), m_IndexBuffer.GetBuffer(), stagingIndexBuffer.GetBufferSize());
	}

	GfxBuffer m_VertexBuffer;
	GfxBuffer m_IndexBuffer;
	uint32_t m_IndexCount;
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