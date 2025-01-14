#ifndef SYSTEMS_HPP
#define SYSTEMS_HPP

#include "Components.h"

namespace Systems
{
	struct UpdateTag{};
	struct FixedUpdateTag{};
	struct LateUpdateTag{};
	struct RenderTag{};

	void Test() {}
}

#endif //SYSTEMS_HPP