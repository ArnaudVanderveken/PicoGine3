#include "pch.h"
#include "CoreSystems.h"

CoreSystems::CoreSystems()
	: m_pWindowManager{ std::make_unique<WindowManager>() }
	, m_pInputManager{ std::make_unique<InputManager>() }
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
            switch (event.type)
            {
            case SDL_QUIT:
                running = false;

            case SDL_KEYDOWN:
                m_pInputManager->KeyDown(event.key.keysym.sym)
            case SDL_KEYUP:
                handleKeyboardEvent(event.key);
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
            //case SDL_MOUSEMOTION:
            case SDL_MOUSEWHEEL:
                handleMouseEvent(event);
                break;

			default:
			    break;
            }
        }

    } while (running);
}
