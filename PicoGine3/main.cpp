#include "pch.h"

#include "CoreSystems.h"

int APIENTRY wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPWSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
	auto& core = CoreSystems::Get();

	if (!core.IsInitialized())
		return 1;

	core.CoreLoop();

	return 0;
}