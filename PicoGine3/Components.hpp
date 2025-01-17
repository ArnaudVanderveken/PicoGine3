#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

#include "flecs.h"

namespace Components
{
	struct Transform
	{
	private:
		XMFLOAT3 m_LocalPosition{ 0.0f, 0.0f, 0.0f };
		XMFLOAT3 m_LocalRotation{ 0.0f, 0.0f, 0.0f };
		XMFLOAT3 m_LocalScale{ 1.0f, 1.0f, 1.0f };
		XMMATRIX m_CachedWorldTransform{};
		bool m_DirtyFlag{ true };

		void UpdateCachedWorldTransform(const flecs::entity& e)
		{
			const XMMATRIX translation{ XMMatrixTranslation(m_LocalPosition.x, m_LocalPosition.y, m_LocalPosition.z) };
			const XMMATRIX rotation{ XMMatrixRotationRollPitchYaw(XMConvertToRadians(m_LocalRotation.x), XMConvertToRadians(m_LocalRotation.y), XMConvertToRadians(m_LocalRotation.z)) };
			const XMMATRIX scale{ XMMatrixScaling(m_LocalScale.x, m_LocalScale.y, m_LocalScale.z) };
			const XMMATRIX localTransform{ translation * rotation * scale };

			const auto& parent{ e.parent() };
			if (parent.is_alive() && parent.has<Transform>())
				m_CachedWorldTransform = localTransform * parent.get_mut<Transform>()->GetWorldTransform(parent);
			else
				m_CachedWorldTransform = localTransform;

			m_DirtyFlag = false;
		}

	public:
		Transform(const XMFLOAT3& position, const XMFLOAT3& rotation, const XMFLOAT3& scale) : m_LocalPosition{ position }, m_LocalRotation{ rotation }, m_LocalScale{ scale } {}
		~Transform() = default;
		Transform(const Transform& other) noexcept : m_LocalPosition{ other.m_LocalPosition }, m_LocalRotation{ other.m_LocalRotation }, m_LocalScale{ other.m_LocalScale } {}
		Transform& operator=(const Transform& other) noexcept
		{
			m_LocalPosition = other.m_LocalPosition;
			m_LocalRotation = other.m_LocalRotation;
			m_LocalScale = other.m_LocalScale;
			m_DirtyFlag = true;

			return *this;
		}
		Transform(Transform&& other) noexcept : m_LocalPosition{ other.m_LocalPosition }, m_LocalRotation{ other.m_LocalRotation }, m_LocalScale{ other.m_LocalScale } {}
		Transform& operator=(Transform&& other) noexcept
		{
			m_LocalPosition = other.m_LocalPosition;
			m_LocalRotation = other.m_LocalRotation;
			m_LocalScale = other.m_LocalScale;
			m_DirtyFlag = true;

			return *this;
		}

		[[nodiscard]] bool IsDirty(const flecs::entity& e) const
		{
			const auto& parent{ e.parent() };
			if (parent.is_alive() && parent.has<Transform>())
				return m_DirtyFlag && parent.get<Transform>()->IsDirty(parent);

			return m_DirtyFlag;
		}

		[[nodiscard]] const XMFLOAT3& GetLocalPosition() const
		{
			return m_LocalPosition;
		}

		[[nodiscard]] const XMFLOAT3& GetLocalRotation() const
		{
			return m_LocalRotation;
		}

		[[nodiscard]] const XMFLOAT3& GetLocalScale() const
		{
			return m_LocalScale;
		}

		void SetLocalPosition(const XMFLOAT3& position)
		{
			m_LocalPosition = position;
			m_DirtyFlag = true;
		}

		void SetLocalPosition(float x, float y, float z)
		{
			m_LocalPosition.x = x;
			m_LocalPosition.y = y;
			m_LocalPosition.z = z;
			m_DirtyFlag = true;
		}

		void AddLocalPosition(const XMFLOAT3& position)
		{
			AddLocalPosition(position.x, position.y, position.z);
		}

		void AddLocalPosition(float x, float y, float z)
		{
			m_LocalPosition.x += x;
			m_LocalPosition.y += y;
			m_LocalPosition.z += z;
			m_DirtyFlag = true;
		}

		void SetLocalRotation(const XMFLOAT3& rotation)
		{
			m_LocalRotation = rotation;
			m_DirtyFlag = true;
		}

		void SetLocalRotation(float x, float y, float z)
		{
			m_LocalRotation.x = x;
			m_LocalRotation.y = y;
			m_LocalRotation.z = z;
			m_DirtyFlag = true;
		}

		void AddLocalRotation(const XMFLOAT3& rotation)
		{
			AddLocalRotation(rotation.x, rotation.y, rotation.z);
		}

		void AddLocalRotation(float x, float y, float z)
		{
			m_LocalRotation.x += x;
			m_LocalRotation.y += y;
			m_LocalRotation.z += z;
			m_DirtyFlag = true;
		}

		void SetLocalScale(const XMFLOAT3& scale)
		{
			m_LocalScale = scale;
			m_DirtyFlag = true;
		}

		void SetLocalScale(float x, float y, float z)
		{
			m_LocalScale.x = x;
			m_LocalScale.y = y;
			m_LocalScale.z = z;
			m_DirtyFlag = true;
		}

		void AddLocalScale(const XMFLOAT3& scale)
		{
			AddLocalScale(scale.x, scale.y, scale.z);
		}

		void AddLocalScale(float x, float y, float z)
		{
			m_LocalScale.x += x;
			m_LocalScale.y += y;
			m_LocalScale.z += z;
			m_DirtyFlag = true;
		}

		[[nodiscard]] const XMMATRIX& GetWorldTransform(const flecs::entity& e)
		{
			if (IsDirty(e))
				UpdateCachedWorldTransform(e);

			return m_CachedWorldTransform;
		}
	};
}

#endif //COMPONENTS_HPP
