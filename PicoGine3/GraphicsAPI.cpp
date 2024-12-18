#include "pch.h"
#include "GraphicsAPI.h"

#if defined(_DX12)

GraphicsAPI::GraphicsAPI() :
    m_IsInitialized{ false }
{
    //m_IsInitialized = true;
}

GraphicsAPI::~GraphicsAPI()
{
}

#elif defined(_VK)

GraphicsAPI::GraphicsAPI() :
	m_IsInitialized{ false }
{
    CreateVkInstance();

	m_IsInitialized = true;
}

GraphicsAPI::~GraphicsAPI()
{
    vkDestroyInstance(m_VkInstance, nullptr);
}

void GraphicsAPI::CreateVkInstance()
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "PicoGine3";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "PicoGine3";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t extensionCount{};
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    std::vector<const char*> instanceExtensions;

    // Required extensions
    const std::vector<const char*> requiredExtensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    };

    for (const auto& requiredExtension : requiredExtensions)
    {
        bool found = false;
        for (const auto& extension : extensions) {
            if (strcmp(extension.extensionName, requiredExtension) == 0)
            {
                instanceExtensions.emplace_back(requiredExtension);
                found = true;
                break;
            }
        }
        if (!found)
			Logger::Get().LogError(std::wstring(L"Vk Instance Extension not found: ") + std::wstring(requiredExtension, requiredExtension + strlen(requiredExtension)));
    }

    // Optional extensions
    const std::vector<const char*> optionalExtensions = {};

    for (const auto& optionalExtension : optionalExtensions)
    {
        for (const auto& extension : extensions) {
            if (strcmp(extension.extensionName, optionalExtension) == 0)
            {
                instanceExtensions.emplace_back(optionalExtension);
                break;
            }
        }
    }

    // VkInstance extensions
    createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();

    createInfo.enabledLayerCount = 0;

    HandleVkResult(vkCreateInstance(&createInfo, nullptr, &m_VkInstance));
}

#endif