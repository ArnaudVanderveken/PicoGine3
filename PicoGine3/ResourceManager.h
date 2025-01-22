#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include "Singleton.h"

#include "ResourceData.h"

class ResourceManager final : public Singleton<ResourceManager>
{
	friend class Singleton<ResourceManager>;
	explicit ResourceManager() = default;

	struct MeshManager
	{
		[[nodiscard]] uint32_t Load(const std::wstring& filename);
		[[nodiscard]] const MeshData& GetMeshData(uint32_t id) const;

	private:
		std::unordered_map<std::wstring, uint32_t> m_LoadedFiles{};
		std::vector<MeshData> m_MeshData{};
	};

public:
	~ResourceManager() override = default;

	ResourceManager(const ResourceManager&) noexcept = delete;
	ResourceManager& operator=(const ResourceManager&) noexcept = delete;
	ResourceManager(ResourceManager&&) noexcept = delete;
	ResourceManager& operator=(ResourceManager&&) noexcept = delete;

	void Initialize();
	[[nodiscard]] bool IsInitialized() const;

	[[nodiscard]] const MeshData& GetMeshData(uint32_t id) const;

	[[nodiscard]] uint32_t LoadMesh(const std::wstring& filename) const;
	//[[nodiscard]] uint32_t LoadTexture(const std::wstring& filename, bool singleChannel = false) const;

private:
	bool m_IsInitialized{};

	std::unique_ptr<MeshManager> m_pMeshManager{};

};

#endif //RESOURCEMANAGER_H