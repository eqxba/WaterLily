#pragma once

#include <volk.h>

class VulkanContext;
class CommandBufferSync;
class Scene;
class Buffer;

using DeviceCommands = std::function<void(VkCommandBuffer)>;

enum class CommandBufferType
{
    eOneTime,
    eLongLived
};

namespace VulkanHelpers
{
    std::vector<VkCommandBuffer> CreateCommandBuffers(VkDevice device, const size_t count, VkCommandPool commandPool);

    void SubmitCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, DeviceCommands commands, 
        const CommandBufferSync& sync);

    VkFence CreateFence(VkDevice device, VkFenceCreateFlags flags);
    std::vector<VkFence> CreateFences(VkDevice device, VkFenceCreateFlags flags, size_t count);

    VkSemaphore CreateSemaphore(VkDevice device);
    std::vector<VkSemaphore> CreateSemaphores(VkDevice device, size_t count);

    void DestroyFences(VkDevice device, std::vector<VkFence>& fences);
    void DestroySemaphores(VkDevice device, std::vector<VkSemaphore>& semaphores);
    
    VkSampler CreateSampler(VkDevice device, VkPhysicalDeviceProperties properties, uint32_t mipLevelsCount);
    void DestroySampler(VkDevice device, VkSampler sampler);

    std::vector<VkDescriptorSet> CreateDescriptorSets(VkDescriptorPool descriptorPool, VkDescriptorSetLayout layout,
        size_t count, VkDevice device);
    void PopulateDescriptorSet(const VkDescriptorSet set, const Buffer& uniformBuffer, const Scene& scene, VkDevice device);
}
