// ReSharper disable CppNonInlineFunctionDefinitionInHeaderFile
#ifndef SYSTEMS_HPP
#define SYSTEMS_HPP

#include "Components.hpp"
#include "Renderer.h"

namespace Systems
{
	using namespace Components;

	void MeshRenderer(flecs::iter& it, Transform* pTransform, Mesh* pMesh)
	{
		auto& renderer{ Renderer::Get() };

		for (const auto i : it)
		{
			auto e{ it.entity(i) };
			renderer.DrawMesh(pMesh->GetMeshDataID(), pMesh->GetMaterialID(), pTransform->GetWorldTransform(e));
		}
	}
}

#endif //SYSTEMS_HPP