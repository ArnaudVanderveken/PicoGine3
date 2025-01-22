#ifndef VERTEX_H
#define VERTEX_H

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
};

struct Vertex2D
{
	XMFLOAT2 m_Position{ 0.0f, 0.0f };
	XMFLOAT3 m_Color{ 1.0f, 1.0f, 1.0f };
	XMFLOAT2 m_Texcoord{ 0.0f, 0.0f };

	bool operator==(const Vertex3D& other) const
	{
		return m_Position.x == other.m_Position.x
			&& m_Position.y == other.m_Position.y
			&& m_Color.x == other.m_Color.x
			&& m_Color.y == other.m_Color.y
			&& m_Color.z == other.m_Color.z
			&& m_Texcoord.x == other.m_Texcoord.x
			&& m_Texcoord.y == other.m_Texcoord.y;
	}
};

#endif //VERTEX_H