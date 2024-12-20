#include "pch.h"
#include "GraphicsAPI.h"

#include "CoreSystems.h"
#include "WindowManager.h"

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
    CreateSurface();
#if defined(_DEBUG)
    SetupDebugMessenger();
#endif //defined(_DEBUG)
    SelectPhysicalDevice();
    CreateLogicalDevice();
    vkGetDeviceQueue(m_VkDevice, m_QueueFamilyIndices.graphicsFamily.value(), 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_VkDevice, m_QueueFamilyIndices.presentFamily.value(), 0, &m_PresentQueue);
    CreateSwapchain();

	m_IsInitialized = true;
}

GraphicsAPI::~GraphicsAPI()
{
    vkDestroySwapchainKHR(m_VkDevice, m_VkSwapChain, nullptr);
    vkDestroyDevice(m_VkDevice, nullptr);
#if defined(_DEBUG)
    CleanupDebugMessenger();
#endif //defined(_DEBUG)
    vkDestroySurfaceKHR(m_VkInstance, m_VkSurface, nullptr);
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

    for (const char* layerName : m_ValidationLayers)
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

    for (const auto& requiredExtension : m_RequiredInstanceExtensions)
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

    for (const auto& optionalExtension : m_OptionalInstanceExtensions)
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

    createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
    createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    PopulateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = &debugCreateInfo;

#else

    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;

#endif //defined(_DEBUG)

    HandleVkResult(vkCreateInstance(&createInfo, nullptr, &m_VkInstance));
}

void GraphicsAPI::CreateSurface()
{
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = WindowManager::Get().GetHWnd();
    createInfo.hinstance = CoreSystems::Get().GetAppHinstance();

    HandleVkResult(vkCreateWin32SurfaceKHR(m_VkInstance, &createInfo, nullptr, &m_VkSurface));
}

void GraphicsAPI::SelectPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_VkInstance, &deviceCount, nullptr);

    if (deviceCount == 0)
        Logger::Get().LogError(L"Failed to find any physical device with Vulkan support!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_VkInstance, &deviceCount, devices.data());

    // Use an ordered map to automatically sort candidates by increasing score
    std::multimap<int, VkPhysicalDevice> candidates;

    for (const auto& device : devices) {
        int score = GradeDevice(device);
        candidates.insert(std::make_pair(score, device));
    }

    // Check if the best candidate is suitable at all
    if (candidates.rbegin()->first > 0)
        m_VkPhysicalDevice = candidates.rbegin()->second;
    
    else
        Logger::Get().LogError(L"Failed to find any suitable physical device.");
    
}

int GraphicsAPI::GradeDevice(VkPhysicalDevice device) const
{
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    if (!deviceFeatures.geometryShader)
        return 0;

    if (!FindQueueFamilies(device).IsComplete())
        return 0;

    if (!CheckDeviceExtensionsSupport(device))
        return 0;

    const SwapChainSupportDetails swapChainSupport{ QuerySwapChainSupport(device) };
    if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty())
        return 0;

    int score{};

    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        score += 1000;

    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

    return score;
}

QueueFamilyIndices GraphicsAPI::FindQueueFamilies(VkPhysicalDevice device) const
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i{};
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphicsFamily = i;

        VkBool32 presentSupport{};
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_VkSurface, &presentSupport);
        if (presentSupport)
            indices.presentFamily = i;

        if (indices.IsComplete())
            break;

        ++i;
    }

    return indices;
}

void GraphicsAPI::CreateLogicalDevice()
{
    m_QueueFamilyIndices = FindQueueFamilies(m_VkPhysicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    const std::set<uint32_t> uniqueQueueFamilies = { m_QueueFamilyIndices.graphicsFamily.value(), m_QueueFamilyIndices.presentFamily.value() };

    constexpr float queuePriority{ 1.0f };
    for (const auto queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

	const VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

#if defined(_DEBUG)
    createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
    createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
#else
    createInfo.enabledLayerCount = 0;
#endif //defined(_DEBUG)

    HandleVkResult(vkCreateDevice(m_VkPhysicalDevice, &createInfo, nullptr, &m_VkDevice));
}

bool GraphicsAPI::CheckDeviceExtensionsSupport(VkPhysicalDevice device) const
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());

    for (const auto& extension : availableExtensions)
        requiredExtensions.erase(extension.extensionName);

    return requiredExtensions.empty();
}

SwapChainSupportDetails GraphicsAPI::QuerySwapChainSupport(VkPhysicalDevice device) const
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_VkSurface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_VkSurface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_VkSurface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_VkSurface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_VkSurface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR GraphicsAPI::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;

    return availableFormats[0];
}

VkPresentModeKHR GraphicsAPI::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return availablePresentMode;

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D GraphicsAPI::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    int width, height;
    WindowManager::Get().GetWindowDimensions(width, height);

    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

void GraphicsAPI::CreateSwapchain()
{
    const SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_VkPhysicalDevice);

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        imageCount = swapChainSupport.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_VkSurface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const uint32_t queueFamilyIndices[] = { m_QueueFamilyIndices.graphicsFamily.value(), m_QueueFamilyIndices.presentFamily.value() };

    if (m_QueueFamilyIndices.graphicsFamily != m_QueueFamilyIndices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    HandleVkResult(vkCreateSwapchainKHR(m_VkDevice, &createInfo, nullptr, &m_VkSwapChain));
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

    HandleVkResult(CreateDebugUtilsMessengerEXT(m_VkInstance, &createInfo, nullptr, &m_VkDebugMessenger));
}

void GraphicsAPI::CleanupDebugMessenger() const
{
    DestroyDebugUtilsMessengerEXT(m_VkInstance, m_VkDebugMessenger, nullptr);
}

#endif //defined(_DEBUG)

#endif
