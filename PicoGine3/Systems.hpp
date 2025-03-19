// ReSharper disable CppNonInlineFunctionDefinitionInHeaderFile
#ifndef SYSTEMS_HPP
#define SYSTEMS_HPP

#include "Components.hpp"
#include "Renderer.h"

namespace Systems
{
	using namespace Components;
	using system_type = void(*)(entt::registry&);

	inline void MeshRenderer(entt::registry& registry)
	{
		auto& renderer{ Renderer::Get() };

		const auto view{ registry.view<Transform, Mesh>() };

		for (const auto entity : view)
		{
			auto& transform{ view.get<Transform>(entity) };
			auto& mesh{ view.get<Mesh>(entity) };

			renderer.DrawMesh(mesh.GetMeshDataID(), mesh.GetMaterialID(), transform.GetWorldTransform(registry));
		}
	};
}

#endif //SYSTEMS_HPP