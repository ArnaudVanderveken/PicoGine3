#include "pch.h"
#include "WindowManager.h"

#include <SDL_syswm.h>

WindowManager::WindowManager()
	: m_IsInitialized{ false }
	, m_pWindow{ nullptr }
	, m_HInstance{}
	, m_WindowHandle{}
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return;
    }

    m_pWindow = SDL_CreateWindow(
        m_WindowName,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1920, // Width
        1080, // Height
        SDL_WINDOW_SHOWN // Flags
    );

    if (!m_pWindow)
    {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return;
    }

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version) // Initialize wmInfo.version
    if (SDL_GetWindowWMInfo(m_pWindow, &wmInfo))
    {
        m_WindowHandle = wmInfo.info.win.window;
        m_HInstance = wmInfo.info.win.hinstance;
    }
    else
    {
        std::cerr << "SDL_GetWindowWMInfo Error: " << SDL_GetError() << std::endl;
        return;
    }

    m_IsInitialized = true;
}

WindowManager::~WindowManager()
{
    if (m_pWindow)
    {
        SDL_DestroyWindow(m_pWindow);
        m_pWindow = nullptr;
    }

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

bool WindowManager::IsInitialized() const
{
	return m_IsInitialized;
}

SDL_Window* WindowManager::GetWindow() const
{
	return m_pWindow;
}

HINSTANCE WindowManager::GetHInstance() const
{
    return m_HInstance;
}

HWND WindowManager::GetHWnd() const
{
    return m_WindowHandle;
}
