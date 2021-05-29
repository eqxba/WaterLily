#include "Engine/Render/Vulkan/Instance.hpp"

#include "Engine/EngineConfig.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"

#include <GLFW/glfw3.h>

namespace InstanceDetails
{	
    static bool LayersSupported(const std::vector<const char*>& layers)
	{  	
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layer : layers)
        {
            const auto isLayerAvailable = [layer](const auto& layerProperties)
            {
                return std::strcmp(layer, layerProperties.layerName) == 0;
            };
        	
            const auto it = std::ranges::find_if(availableLayers, isLayerAvailable);
        	
            if (it == availableLayers.end())
            {
                LogE << "Layer not supported: " << layer << std::endl;
                return false;
            }
        }

        return true;
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
        else
        {
            LogW << message << std::endl;
        }

        return VK_FALSE;
    }
	
	// TODO: Find out the right way to create it in place (i.e. Type val = GetVal() with no stack copying and & param)
	static VkDebugUtilsMessengerCreateInfoEXT GetDebugMessengerCreateInfo()
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = DebugCallback;
        createInfo.pUserData = nullptr; // Optional

        return createInfo;
    }

	static VkDebugUtilsMessengerEXT CreateDebugMessenger(VkInstance instance)
    {
	    if constexpr (!VulkanConfig::useValidation)
	    {
            return VK_NULL_HANDLE;
	    }

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

    // TODO: Check for extensions support
    std::vector<const char*> requiredExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    std::vector<const char*> requiredLayers;

	if constexpr (VulkanConfig::useValidation)
	{
        requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        requiredLayers.emplace_back("VK_LAYER_KHRONOS_validation");
	}

    const bool layersSupported = LayersSupported(requiredLayers);
    Assert(layersSupported);
	
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &applicationInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());;
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size());
    createInfo.ppEnabledLayerNames = requiredLayers.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if constexpr (VulkanConfig::useValidation)
	{
        debugCreateInfo = GetDebugMessengerCreateInfo();
        createInfo.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
    }
	
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    Assert(result == VK_SUCCESS);

    volkLoadInstance(instance);

    debugMessenger = CreateDebugMessenger(instance);
}

Instance::~Instance()
{
	if (debugMessenger != VK_NULL_HANDLE)
	{
        vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}
	
    vkDestroyInstance(instance, nullptr);
}
