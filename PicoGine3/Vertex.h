#ifndef VERTEX_H
#define VERTEX_H

#pragma region vertex3d

struct Vertex3D
{
	XMFLOAT3 m_Position{ 0.0f, 0.0f, 0.0f };
	XMFLOAT3 m_Color{ 1.0f, 1.0f, 1.0f };
	XMFLOAT2 m_Texcoord{ 0.0f, 0.0f };

	bool operator==(const Vertex3D& other) const
	{
		return m_Position.x == other.m_Position.x
			&& m_Position.y == other.m_Position.y
			&& m_Position.z == other.m_Position.z
			&& m_Color.x == other.m_Color.x
			&& m_Color.y == other.m_Color.y
			&& m_Color.z == other.m_Color.z
			&& m_Texcoord.x == other.m_Texcoord.x
			&& m_Texcoord.y == other.m_Texcoord.y;
	}

#if defined(_VK)
	static const VkVertexInputBindingDescription& GetBindingDescription()
	{
		static VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex3D);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static const std::array<VkVertexInputAttributeDescription, 3>& GetAttributeDescriptions()
	{
		static std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex3D, m_Position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex3D, m_Color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex3D, m_Texcoord);

		return attributeDescriptions;
	}
#elif defined(_DX)

#endif
};

template <>
struct std::hash<Vertex3D>
{
	std::size_t operator()(const Vertex3D& vertex) const noexcept
	{
		const std::size_t h1 = std::hash<float>()(vertex.m_Position.x) ^ (std::hash<float>()(vertex.m_Position.y) << 1) ^ (std::hash<float>()(vertex.m_Position.z) << 2);
		const std::size_t h2 = std::hash<float>()(vertex.m_Color.x) ^ (std::hash<float>()(vertex.m_Color.y) << 1) ^ (std::hash<float>()(vertex.m_Color.z) << 2);
		const std::size_t h3 = std::hash<float>()(vertex.m_Texcoord.x) ^ (std::hash<float>()(vertex.m_Texcoord.y) << 1);
		return h1 ^ h2 ^ h3;
	}
};

#pragma endregion

#pragma region vertex2d

struct Vertex2D
{
	XMFLOAT2 m_Position{ 0.0f, 0.0f };
	XMFLOAT3 m_Color{ 1.0f, 1.0f, 1.0f };
	XMFLOAT2 m_Texcoord{ 0.0f, 0.0f };

	bool operator==(const Vertex2D& other) const
	{
		return m_Position.x == other.m_Position.x
			&& m_Position.y == other.m_Position.y
			&& m_Color.x == other.m_Color.x
			&& m_Color.y == other.m_Color.y
			&& m_Color.z == other.m_Color.z
			&& m_Texcoord.x == other.m_Texcoord.x
			&& m_Texcoord.y == other.m_Texcoord.y;
	}

#if defined(_VK)
	static const VkVertexInputBindingDescription& GetBindingDescription()
	{
		static VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex2D);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static const std::array<VkVertexInputAttributeDescription, 3>& GetAttributeDescriptions()
	{
		static std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex2D, m_Position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex2D, m_Color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex2D, m_Texcoord);

		return attributeDescriptions;
	}
#elif defined(_DX)

#endif
};

template <>
struct std::hash<Vertex2D>
{
	std::size_t operator()(const Vertex2D& vertex) const noexcept
	{
		const std::size_t h1 = std::hash<float>()(vertex.m_Position.x) ^ (std::hash<float>()(vertex.m_Position.y) << 1);
		const std::size_t h2 = std::hash<float>()(vertex.m_Color.x) ^ (std::hash<float>()(vertex.m_Color.y) << 1) ^ (std::hash<float>()(vertex.m_Color.z) << 2);
		const std::size_t h3 = std::hash<float>()(vertex.m_Texcoord.x) ^ (std::hash<float>()(vertex.m_Texcoord.y) << 1);
		return h1 ^ h2 ^ h3;
	}
};

#pragma endregion

#endif //VERTEX_H