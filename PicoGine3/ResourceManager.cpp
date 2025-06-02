#include "pch.h"
#include "ResourceManager.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Renderer.h"
#include "Vertex.h"


uint32_t ResourceManager::MeshManager::Load(const std::wstring& filename)
{
	if (!m_LoadedFiles.contains(filename))
	{
		std::vector<Vertex3D> vertices{};
		std::vector<uint32_t> indices{};

		Assimp::Importer importer;

		const aiScene* scene{ importer.ReadFile("Resources/Models/viking_room.obj", aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_FlipUVs) };

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
			Logger::Get().LogError(L"Error::Assimp: " + StrUtils::cstr2stdwstr(importer.GetErrorString()) + L"\n");

		const aiMesh* mesh{ scene->mMeshes[0] };
		indices.reserve(mesh->mNumVertices); // Can't reserve for vertices since size is unknown
		std::unordered_map<Vertex3D, uint32_t> uniqueVertices;

		for (uint32_t i{}; i < mesh->mNumVertices; ++i)
		{
			Vertex3D vertex{};

			// Extract position
			vertex.m_Position.x = mesh->mVertices[i].x;
			vertex.m_Position.y = mesh->mVertices[i].y;
			vertex.m_Position.z = mesh->mVertices[i].z;

			// Extract UV coordinates (if available)
			if (mesh->mTextureCoords[0])
			{
				vertex.m_Texcoord.x = mesh->mTextureCoords[0][i].x;
				vertex.m_Texcoord.y = mesh->mTextureCoords[0][i].y;
			}

			if (!uniqueVertices.contains(vertex))
			{
				// Add new vertex
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.emplace_back(vertex);
			}

			indices.emplace_back(uniqueVertices[vertex]);
		}

		m_LoadedFiles[filename] = static_cast<uint32_t>(m_MeshData.size());

		const auto graphicsAPI{ Renderer::Get().GetGraphicsAPI() };

		m_MeshData.emplace_back(MeshData{ graphicsAPI, vertices, indices });
	}

	return m_LoadedFiles[filename];
}

const MeshData& ResourceManager::MeshManager::GetMeshData(uint32_t id) const
{
	assert(id < m_MeshData.size() && L"MeshData fetch with id out of range!");
	return m_MeshData[id];
}

void ResourceManager::MeshManager::ReleaseGPUBuffers()
{
	m_MeshData.clear();
}

void ResourceManager::Initialize()
{
	m_pMeshManager = std::make_unique<MeshManager>();

	m_IsInitialized = true;
}

bool ResourceManager::IsInitialized() const
{
	return m_IsInitialized;
}

const MeshData& ResourceManager::GetMeshData(uint32_t id) const
{
	return m_pMeshManager->GetMeshData(id);
}

uint32_t ResourceManager::LoadMesh(const std::wstring& filename) const
{
	return m_pMeshManager->Load(filename);
}

void ResourceManager::ReleaseGPUBuffers() const
{
	m_pMeshManager->ReleaseGPUBuffers();
}

//uint32_t ResourceManager::LoadTexture(const std::wstring& filename, bool singleChannel) const
//{
//	return 0;
//}
