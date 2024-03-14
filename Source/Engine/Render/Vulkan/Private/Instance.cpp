#include "Engine/Render/Vulkan/Instance.hpp"

#include "Engine/EngineConfig.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"

#include <GLFW/glfw3.h>

namespace InstanceDetails
{   
    static std::vector<VkExtensionProperties> GetAvailableExtensionsProperties()
    {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensionsProperties(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensionsProperties.data());

        return availableExtensionsProperties;
    }

    static bool ExtensionsSupported(const std::vector<const char*>& extensions)
    {
        std::vector<VkExtensionProperties> availableExtensionsProperties = GetAvailableExtensionsProperties();

        const auto isSupported = [&](const char* extension) {
            const bool isSupported = std::ranges::any_of(availableExtensionsProperties, [=](const auto& properties) {
                return std::strcmp(extension, properties.extensionName) == 0;
            });

            if (!isSupported)
            {
                LogE << "Extension not supported: " << extension << '\n';
            }

            return isSupported;
        };

        return std::ranges::all_of(extensions, isSupported);
    }

    static std::vector<VkLayerProperties> GetAvailableLayersProperties()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayersProperties(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayersProperties.data());

        return availableLayersProperties;
    }

    static bool LayersSupported(const std::vector<const char*>& layers)
    {       
        std::vector<VkLayerProperties> availableLayersProperties = GetAvailableLayersProperties();

        const auto isSupported = [&](const char* layer) {
            const bool isSupported = std::ranges::any_of(availableLayersProperties, [=](const auto& properties) {
                return std::strcmp(layer, properties.layerName) == 0;
            });

            if (!isSupported)
            {
                LogE << "Layer not supported: " << layer << '\n';
            }

            return isSupported;
        };

        return std::ranges::all_of(layers, isSupported);
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        std::string message(pCallbackData->pMessage);
        message = message.substr(0, message.find("(http"));
    	
    	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    	{
            LogE << message << std::endl;
            Assert(false);
    	}
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            LogW << message << std::endl;
        }

        return VK_FALSE;
    }
	
	static VkDebugUtilsMessengerCreateInfoEXT GetDebugMessengerCreateInfo()
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = DebugCallback;
        createInfo.pUserData = nullptr;

        return createInfo;
    }

	static VkDebugUtilsMessengerEXT CreateDebugMessenger(VkInstance instance)
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfo = GetDebugMessengerCreateInfo();      
        VkDebugUtilsMessengerEXT debugMessenger;
    	
        const VkResult result = vkCreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger);
        Assert(result == VK_SUCCESS);

        return debugMessenger;
    }
}

Instance::Instance()
{
    using namespace InstanceDetails;
	
	// It's okay due to value&zero initialization to skip pNext (it's gonna be nullptr)
    VkApplicationInfo applicationInfo{};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pApplicationName = EngineConfig::engineName;
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pEngineName = EngineConfig::engineName;
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.apiVersion = VK_API_VERSION_1_2;
	
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> requiredExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    std::vector<const char*> requiredLayers;

	if constexpr (VulkanConfig::useValidation)
	{
        requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        requiredLayers.emplace_back("VK_LAYER_KHRONOS_validation");
	}
	
    const bool extensionsSupported = ExtensionsSupported(requiredExtensions);
    Assert(extensionsSupported);
	
    const bool layersSupported = LayersSupported(requiredLayers);
    Assert(layersSupported);
	
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &applicationInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());;
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size());
    createInfo.ppEnabledLayerNames = requiredLayers.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if constexpr (VulkanConfig::useValidation)
	{
        debugCreateInfo = GetDebugMessengerCreateInfo();
        createInfo.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
    }
	
    // A debug messenger can be enabled for this call as well but YAGNI
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    Assert(result == VK_SUCCESS);

    volkLoadInstanceOnly(instance); 

    if constexpr (VulkanConfig::useValidation)
    {
        debugMessenger = CreateDebugMessenger(instance);
    }    
}

Instance::~Instance()
{
	if (debugMessenger != VK_NULL_HANDLE)
	{
        vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}
	
    vkDestroyInstance(instance, nullptr);
}
