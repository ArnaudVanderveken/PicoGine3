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

#if defined(_DEBUG)
    SetupDebugMessenger();
#endif //defined(_DEBUG)

	m_IsInitialized = true;
}

GraphicsAPI::~GraphicsAPI()
{
#if defined(_DEBUG)
    CleanupDebugMessenger();
#endif //defined(_DEBUG)

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
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#if defined(_DEBUG)
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif //defined(_DEBUG)
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

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    PopulateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = &debugCreateInfo;

#else

    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;

#endif //defined(_DEBUG)

    HandleVkResult(vkCreateInstance(&createInfo, nullptr, &m_VkInstance));
}

#if defined(_DEBUG)

VKAPI_ATTR VkBool32 VKAPI_CALL GraphicsAPI::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT /*messageType*/, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* /*pUserData*/)
{
    if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		Logger::Get().LogWarning(StrUtils::cstr2stdwstr(pCallbackData->pMessage));

    else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        Logger::Get().LogError(StrUtils::cstr2stdwstr(pCallbackData->pMessage));

    return VK_FALSE;
}

void GraphicsAPI::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = GraphicsAPI::DebugCallback;
    createInfo.pUserData = nullptr; // Optional
}

VkResult GraphicsAPI::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    if (const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT")); func != nullptr)
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void GraphicsAPI::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    if (const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT")); func != nullptr)
        func(instance, debugMessenger, pAllocator);
    
}

void GraphicsAPI::SetupDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    PopulateDebugMessengerCreateInfo(createInfo);

    HandleVkResult(CreateDebugUtilsMessengerEXT(m_VkInstance, &createInfo, nullptr, &m_DebugMessenger));
}

void GraphicsAPI::CleanupDebugMessenger() const
{
    DestroyDebugUtilsMessengerEXT(m_VkInstance, m_DebugMessenger, nullptr);
}

#endif //defined(_DEBUG)

#endif
