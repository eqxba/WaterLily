#pragma once

#include "Engine/Render/Vulkan/VulkanUtils.hpp"
#include "Engine/Render/Vulkan/Synchronization/CommandBufferSync.hpp"

#include <volk.h>

class VulkanContext;

struct QueueFamilyIndices
{
    uint32_t graphicsAndComputeFamily;
    uint32_t presentFamily;
};

struct Queues
{    
    VkQueue graphicsAndCompute;
    VkQueue present;
    QueueFamilyIndices familyIndices;
};

struct DeviceProperties
{
    VkPhysicalDeviceProperties physicalProperties = {};
    VkSampleCountFlagBits maxSampleCount = VK_SAMPLE_COUNT_1_BIT;
    bool meshShadersSupported = false;
    bool pipelineStatisticsQuerySupported = false;
    bool drawIndirectCountSupported = false;
    bool samplerFilterMinmaxSupported = false;
};

class Device
{
public:
    Device(const VulkanContext& aVulkanContext);
    ~Device();

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    Device(Device&&) = delete;
    Device& operator=(Device&&) = delete;

    void WaitIdle() const;

    VkCommandPool GetCommandPool(CommandBufferType type) const;
    void ExecuteOneTimeCommandBuffer(const DeviceCommands& commands) const;
    
    VkPhysicalDevice GetPhysicalDevice() const
    {
        return physicalDevice;
    }

    const DeviceProperties& GetProperties() const
    {
        return properties;
    }

    const Queues& GetQueues() const
    {
        return queues;
    }
    
    operator VkDevice() const
    {
        return device;
    }

private:
    void InitProperties();
    
    const VulkanContext& vulkanContext;

    VkDevice device;
    VkPhysicalDevice physicalDevice;

    DeviceProperties properties;

    Queues queues;

    mutable std::unordered_map<CommandBufferType, VkCommandPool> commandPools;

    VkCommandBuffer oneTimeCommandBuffer;
    CommandBufferSync oneTimeCommandBufferSync;
};
