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

#if defined(_DEBUG)
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
    };

    for (const char* layerName : validationLayers)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }
        if (!layerFound)
            Logger::Get().LogError(std::wstring(L"Validation layer not found: ") + StrUtils::cstr2stdwstr(layerName));
    }
#endif //defined(_DEBUG)

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
        bool extensionFound = false;
        for (const auto& extension : extensions)
        {
            if (strcmp(extension.extensionName, requiredExtension) == 0)
            {
                instanceExtensions.emplace_back(requiredExtension);
                extensionFound = true;
                break;
            }
        }
        if (!extensionFound)
			Logger::Get().LogError(std::wstring(L"Vk Instance Extension not found: ") + StrUtils::cstr2stdwstr(requiredExtension));
    }

    // Optional extensions
    const std::vector<const char*> optionalExtensions = {};

    for (const auto& optionalExtension : optionalExtensions)
    {
        for (const auto& extension : extensions)
        {
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

#if defined(_DEBUG)
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
#else
    createInfo.enabledLayerCount = 0;
#endif //defined(_DEBUG)

    HandleVkResult(vkCreateInstance(&createInfo, nullptr, &m_VkInstance));
}

#endif