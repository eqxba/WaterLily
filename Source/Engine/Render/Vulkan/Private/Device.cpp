#include "Engine/Render/Vulkan/Device.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanUtils.hpp"

namespace DeviceDetails
{
    static bool ExtensionSupported(const std::vector<VkExtensionProperties>& availableExtensionsProperties,
        const char* extension, bool logError = false)
    {
        const bool isSupported = std::ranges::any_of(availableExtensionsProperties, [=](const auto& properties) {
            return std::strcmp(extension, properties.extensionName) == 0;
        });
        
        if (!isSupported && logError)
        {
            LogE << "Extension not supported: " << extension << '\n';
        }
        
        return isSupported;
    }

    static std::vector<VkExtensionProperties> GetExtensionsProperties(VkPhysicalDevice device)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensionsProperties(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensionsProperties.data());
        
        return availableExtensionsProperties;
    }
    
    static bool ExtensionsSupported(VkPhysicalDevice device, const std::span<const char* const> extensions,
        bool logError = true)
    {
        std::vector<VkExtensionProperties> availableExtensionsProperties = GetExtensionsProperties(device);

        const auto isSupported = [&](const char* extension) {
            return ExtensionSupported(availableExtensionsProperties, extension, logError);
        };

        return std::ranges::all_of(extensions, isSupported);    
    }

    // TODO: Handle compute queue separatelly
    static std::optional<uint32_t> FindGraphicsAndComputetQueueFamilyIndex(const std::vector<VkQueueFamilyProperties>& queueFamilies)
    {
        const auto isGraphicsAndComputeFamily = [](const VkQueueFamilyProperties& properties) {
            return properties.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
        };

        const auto it = std::ranges::find_if(queueFamilies, isGraphicsAndComputeFamily);

        return it == queueFamilies.end() ? std::nullopt
            : std::optional(static_cast<uint32_t>(std::distance(queueFamilies.begin(), it)));
    }

    static std::optional<uint32_t> FindPresentQueueFamilyIndex(const std::vector<VkQueueFamilyProperties>& queueFamilies,
        VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        for (uint32_t i = 0; i < queueFamilies.size(); ++i)
        {
            VkBool32 presentSupport = false;

            const VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            Assert(result == VK_SUCCESS);

            if (presentSupport)
            {
                return i;
            }
        }

        return std::nullopt;
    }
    
    static QueueFamilyIndices GetQueueFamilyIndices(const VulkanContext& vulkanContext, VkPhysicalDevice device)
    {                        
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        std::optional<uint32_t> graphicsAndComputeFamily = FindGraphicsAndComputetQueueFamilyIndex(queueFamilies);
        Assert(graphicsAndComputeFamily.has_value());

        std::optional<uint32_t> presentFamily = FindPresentQueueFamilyIndex(queueFamilies, device, vulkanContext.GetSurface());
        Assert(presentFamily.has_value());        
        
        return { graphicsAndComputeFamily.value(), presentFamily.value()};
    }

    static Queues GetQueues(const VulkanContext& vulkanContext, VkPhysicalDevice physicalDevice, VkDevice device)
    {
        Queues queues{};
        queues.familyIndices = DeviceDetails::GetQueueFamilyIndices(vulkanContext, physicalDevice);
        
        vkGetDeviceQueue(device, queues.familyIndices.graphicsAndComputeFamily, 0, &queues.graphicsAndCompute);
        vkGetDeviceQueue(device, queues.familyIndices.presentFamily, 0, &queues.present);

        return queues;
    }
    
    static bool IsPhysicalDeviceSuitable(VkPhysicalDevice device)
    {
        return ExtensionsSupported(device, std::span(VulkanConfig::requiredDeviceExtensions));
    }

    // TODO: (low priority) device selection based on some kind of score (do i really need this?)
    static std::tuple<VkPhysicalDevice, VkPhysicalDeviceProperties> SelectPhysicalDevice(
        const VulkanContext& vulkanContext)
    {
        const VkInstance vkInstance = vulkanContext.GetInstance();

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);
        Assert(deviceCount != 0);
        
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data());

        const auto it = std::ranges::find_if(devices, IsPhysicalDeviceSuitable);
        Assert(it != devices.end());

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(*it, &deviceProperties);

        LogI << "Selected physical device: " << deviceProperties.deviceName << std::endl;

        return { *it, deviceProperties };
    }

    static VkDevice CreateLogicalDevice(const VulkanContext& vulkanContext, VkPhysicalDevice physicalDevice,
        const DeviceProperties& properties)
    {
        using namespace VulkanConfig;

        QueueFamilyIndices indices = GetQueueFamilyIndices(vulkanContext, physicalDevice);

        float queuePriority = 1.0f;
                
        std::set<uint32_t> uniqueQueueFamilyIndices = { indices.graphicsAndComputeFamily, indices.presentFamily };
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        queueCreateInfos.reserve(uniqueQueueFamilyIndices.size());

        const auto createQueueCreateInfo = [&](uint32_t queueFamilyIndex) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;            

            return queueCreateInfo;
        }; 

        std::ranges::transform(uniqueQueueFamilyIndices, std::back_inserter(queueCreateInfos), createQueueCreateInfo);
        
        // Optional features:
        
        VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT,
            .taskShader = VK_TRUE,
            .meshShader = VK_TRUE };
        
        void* optionalFeaturesChain = properties.meshShadersSupported
            ? reinterpret_cast<void*>(&meshShaderFeatures) : nullptr;
        
        // Required features:

        VkPhysicalDeviceFeatures deviceFeatures = {
            .multiDrawIndirect = VK_TRUE,
            .samplerAnisotropy = VK_TRUE,
            .pipelineStatisticsQuery = VK_TRUE };

        VkPhysicalDeviceVulkan11Features deviceFeatures11 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
            .pNext = optionalFeaturesChain,
            .storageBuffer16BitAccess = VK_TRUE,
            .shaderDrawParameters = VK_TRUE };

        VkPhysicalDeviceVulkan12Features deviceFeatures12 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
            .pNext = &deviceFeatures11,
            .drawIndirectCount = VK_TRUE,
            .storageBuffer8BitAccess = VK_TRUE,
            .shaderInt8 = VK_TRUE };

        VkPhysicalDeviceFeatures2 deviceFeatures2 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &deviceFeatures12,
            .features = deviceFeatures };

        std::vector enabledExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());

        if (properties.meshShadersSupported)
        {
            enabledExtensions.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
        }

        VkDeviceCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &deviceFeatures2,
            .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
            .pQueueCreateInfos = queueCreateInfos.data(),
            .enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size()),
            .ppEnabledExtensionNames = enabledExtensions.data() };

        VkDevice device;
        const VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
        Assert(result == VK_SUCCESS);

        return device;
    }

    static VkCommandPool CreateCommandPool(VkDevice device, VkCommandPoolCreateFlags flags, uint32_t queueFamilyIndex)
    {        
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndex;
        poolInfo.flags = flags;

        VkCommandPool commandPool;
        const VkResult result = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
        Assert(result == VK_SUCCESS);

        return commandPool;
    }

    static VkSampleCountFlagBits GetMaxSampleCount(const VkPhysicalDeviceProperties& physicalDeviceProperties)
    {
        VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
            physicalDeviceProperties.limits.framebufferDepthSampleCounts;

        std::array countsToConsider = { VK_SAMPLE_COUNT_64_BIT, VK_SAMPLE_COUNT_32_BIT, VK_SAMPLE_COUNT_16_BIT,
            VK_SAMPLE_COUNT_8_BIT, VK_SAMPLE_COUNT_4_BIT, VK_SAMPLE_COUNT_2_BIT };

        auto result = std::ranges::find_if(countsToConsider, [=](VkSampleCountFlagBits count) {
            return counts & count;
        });

        return result != countsToConsider.end() ? *result : VK_SAMPLE_COUNT_1_BIT;
    }
}

Device::Device(const VulkanContext& aVulkanContext)
    : vulkanContext{aVulkanContext}
{
    using namespace DeviceDetails;

    std::tie(physicalDevice, properties.physicalProperties) = SelectPhysicalDevice(vulkanContext);
    
    InitProperties();
    
    device = CreateLogicalDevice(vulkanContext, physicalDevice, properties);

    volkLoadDevice(device);

    queues = DeviceDetails::GetQueues(vulkanContext, physicalDevice, device);

    commandPools.emplace(CommandBufferType::eLongLived, 
        CreateCommandPool(device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queues.familyIndices.graphicsAndComputeFamily));
    commandPools.emplace(CommandBufferType::eOneTime,
        CreateCommandPool(device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | 
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, queues.familyIndices.graphicsAndComputeFamily));

    oneTimeCommandBuffer = VulkanUtils::CreateCommandBuffers(device, 1, commandPools[CommandBufferType::eOneTime])[0];
    oneTimeCommandBufferSync = CommandBufferSync{ {}, {}, {}, VulkanUtils::CreateFence(device, {}), device };
}

Device::~Device()
{
    std::ranges::for_each(commandPools, [&](auto& entry) {
        vkDestroyCommandPool(device, entry.second, nullptr);
    });

    // Gotta call it explicitly here cause on scope exit it's called by compiler and there's already no device
    oneTimeCommandBufferSync.~CommandBufferSync();

    vkDestroyDevice(device, nullptr);
}

void Device::WaitIdle() const
{
    vkDeviceWaitIdle(device);
}

VkCommandPool Device::GetCommandPool(CommandBufferType type) const
{
    return commandPools[type];
}

void Device::ExecuteOneTimeCommandBuffer(const DeviceCommands& commands) const
{
    VulkanUtils::SubmitCommandBuffer(oneTimeCommandBuffer, queues.graphicsAndCompute, commands, oneTimeCommandBufferSync);

    VkFence fence = oneTimeCommandBufferSync.GetFence();
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

    const VkResult result = vkResetFences(device, 1, &fence);
    Assert(result == VK_SUCCESS);
}

void Device::InitProperties()
{
    using namespace DeviceDetails;
    
    std::vector<VkExtensionProperties> availableExtensionsProperties = GetExtensionsProperties(physicalDevice);
    
    properties.maxSampleCount = GetMaxSampleCount(properties.physicalProperties);
    properties.meshShadersSupported = ExtensionSupported(availableExtensionsProperties, VK_EXT_MESH_SHADER_EXTENSION_NAME);
}
