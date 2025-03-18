// ReSharper disable CppNonInlineFunctionDefinitionInHeaderFile
#ifndef SYSTEMS_HPP
#define SYSTEMS_HPP

#include "Components.hpp"
#include "Renderer.h"

namespace Systems
{
	using namespace Components;

	void MeshRenderer(flecs::iter& it)
	{
		auto& renderer{ Renderer::Get() };
		const auto transforms{ it.field<Transform>(0) };
		const auto meshes{ it.field<Mesh>(1) };

		for (const auto i : it)
		{
			auto e{ it.entity(i) };
			renderer.DrawMesh(meshes[i].GetMeshDataID(), meshes[i].GetMaterialID(), transforms[i].GetWorldTransform(e));
		}
	}
}

#endif //SYSTEMS_HPP