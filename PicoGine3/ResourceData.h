#ifndef RESOURCEDATA_H
#define RESOURCEDATA_H

#include "GfxBuffer.h"
#include "GfxImage.h"
#include "GraphicsAPI.h"
#include "Vertex.h"

struct MeshData
{
	explicit MeshData(GraphicsAPI* pGraphicsAPI, const std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& indices) :
		m_pVertexBuffer{ std::make_unique<GfxBuffer>(pGraphicsAPI, sizeof(Vertex3D), vertices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) },
		m_pIndexBuffer{ std::make_unique<GfxBuffer>(pGraphicsAPI, sizeof(uint32_t), indices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) },
		m_IndexCount{ static_cast<uint32_t>(indices.size()) }
	{
		const auto pDevice{ pGraphicsAPI->GetGfxDevice() };
		const auto cmdBuffer{ pGraphicsAPI->GetCurrentCommandBuffer().GetCmdBuffer() };
		GfxBuffer stagingVertexBuffer{ pGraphicsAPI, sizeof(Vertex3D), vertices.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

		stagingVertexBuffer.Map();
		stagingVertexBuffer.WriteToBuffer(vertices.data());
		stagingVertexBuffer.Unmap();

		GfxBuffer stagingIndexBuffer{ pGraphicsAPI, sizeof(uint32_t), indices.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

		stagingIndexBuffer.Map();
		stagingIndexBuffer.WriteToBuffer(indices.data());
		stagingIndexBuffer.Unmap();

		pDevice->CopyBuffer(cmdBuffer, stagingVertexBuffer.GetBuffer(), m_pVertexBuffer->GetBuffer(), stagingVertexBuffer.GetBufferSize());
		pDevice->CopyBuffer(cmdBuffer, stagingIndexBuffer.GetBuffer(), m_pIndexBuffer->GetBuffer(), stagingIndexBuffer.GetBufferSize());
	}

	std::unique_ptr<GfxBuffer> m_pVertexBuffer;
	std::unique_ptr<GfxBuffer> m_pIndexBuffer;
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