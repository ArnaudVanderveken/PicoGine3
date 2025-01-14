#include "pch.h"

#include "CoreSystems.h"

int APIENTRY wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPWSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
	// Check for DirectX Math library support.
	if (!XMVerifyCPUSupport())
	{
		::MessageBoxA(nullptr, "Failed to verify DirectX Math library support.", "Error", MB_OK | MB_ICONERROR);
		return 1;
	}

	// Windows 10 Creators update adds Per Monitor V2 DPI awareness context.
	// Using this awareness context allows the client area of the window to achieve 100% scaling
	// while still allowing non-client window content to be rendered in a DPI sensitive fashion.
	::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	auto& core = CoreSystems::Get();

	core.Initialize();

	if (!core.IsInitialized())
		return 1;

	return core.CoreLoop();
}