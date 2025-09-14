#ifndef RESOURCEDATA_H
#define RESOURCEDATA_H

#include "GfxStructs.h"
#include "GraphicsAPI.h"
#include "Vertex.h"

struct MeshData
{
	explicit MeshData(GraphicsAPI* pGraphicsAPI, const std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& indices) :
		m_pGraphicsAPI{ pGraphicsAPI },
		m_IndexCount{ static_cast<uint32_t>(indices.size()) }
	{
		const auto pDevice{ m_pGraphicsAPI->GetGfxDevice() };
		const auto cmdBuffer{ m_pGraphicsAPI->GetCurrentCommandBuffer().GetCmdBuffer() };

		const BufferDesc vbDesc{
			.m_Usage = BufferUsageBits_Storage | BufferUsageBits_Vertex,
			.m_Storage = StorageType_Device,
			.m_Size = sizeof(Vertex3D) * vertices.size()
		};
		m_pVertexBufferHandle = m_pGraphicsAPI->AcquireBuffer(vbDesc);
		const auto& vertexBuffer{ m_pGraphicsAPI->GetBuffer(m_pVertexBufferHandle) };
		assert(vertexBuffer && L"Unable to create vertex buffer");

		const BufferDesc ibDesc{
			.m_Usage = BufferUsageBits_Storage | BufferUsageBits_Index,
			.m_Storage = StorageType_Device,
			.m_Size = sizeof(uint32_t) * indices.size()
		};
		m_pIndexBufferHandle = m_pGraphicsAPI->AcquireBuffer(ibDesc);
		const auto& indexBuffer{ m_pGraphicsAPI->GetBuffer(m_pIndexBufferHandle) };
		assert(indexBuffer && L"Unable to create index buffer");

		// TODO: move staging buffer to a dedicated pool to avoid useless acquire/destroy on the main pool.
		const BufferDesc svbDesc{
			.m_Usage = BufferUsageBits_Storage,
			.m_Storage = StorageType_HostVisible,
			.m_Size = sizeof(Vertex3D) * vertices.size(),
			.m_Data = vertices.data()
		};
		const BufferHandle stagingVertexBufferHandle{ m_pGraphicsAPI->AcquireBuffer(svbDesc) };
		const auto& stagingVertexBuffer{ m_pGraphicsAPI->GetBuffer(stagingVertexBufferHandle) };
		assert(stagingVertexBuffer && L"Unable to create staging vertex buffer");

		const BufferDesc sibDesc{
			.m_Usage = BufferUsageBits_Storage,
			.m_Storage = StorageType_HostVisible,
			.m_Size = sizeof(uint32_t) * indices.size(),
			.m_Data = indices.data()
		};
		const BufferHandle stagingIndexBufferHandle{ m_pGraphicsAPI->AcquireBuffer(sibDesc) };
		const auto& stagingIndexBuffer{ m_pGraphicsAPI->GetBuffer(stagingIndexBufferHandle) };
		assert(stagingIndexBuffer && L"Unable to create staging vertex buffer");

		pDevice->CopyBuffer(cmdBuffer, stagingVertexBuffer->m_VkBuffer, vertexBuffer->m_VkBuffer, stagingVertexBuffer->m_BufferSize);
		pDevice->CopyBuffer(cmdBuffer, stagingIndexBuffer->m_VkBuffer, indexBuffer->m_VkBuffer, stagingIndexBuffer->m_BufferSize);

		m_pGraphicsAPI->Destroy(stagingVertexBufferHandle);
		m_pGraphicsAPI->Destroy(stagingIndexBufferHandle);
	}

	~MeshData()
	{
		m_pGraphicsAPI->Destroy(m_pVertexBufferHandle);
		m_pGraphicsAPI->Destroy(m_pIndexBufferHandle);
	}

	MeshData(const MeshData&) noexcept = delete;
	MeshData& operator=(const MeshData&) noexcept = delete;
	MeshData(MeshData&&) noexcept = delete;
	MeshData& operator=(MeshData&&) noexcept = delete;

	GraphicsAPI* m_pGraphicsAPI;
	BufferHandle m_pVertexBufferHandle;
	BufferHandle m_pIndexBufferHandle;
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