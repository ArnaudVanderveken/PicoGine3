#include "pch.h"
#include "CoreSystems.h"

CoreSystems::CoreSystems()
	: m_pWindowManager{ std::make_unique<WindowManager>() }
{

}

bool CoreSystems::IsInitialized() const
{
	return m_pWindowManager->IsInitialized();
}

WindowManager* CoreSystems::GetWindowManager() const
{
	return m_pWindowManager.get();
}

void CoreSystems::CoreLoop()
{
    bool running{ true };
    SDL_Event event;

    do
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                running = false;


        }
    } while (running);
}
