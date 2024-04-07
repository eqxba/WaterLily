#pragma once

#include <volk.h>

class VulkanContext;
class CommandBufferSync;

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
    std::vector<VkSemaphore> CreateSemaphores(VkDevice device, size_t count);

    void DestroyFences(VkDevice device, std::vector<VkFence>& fences);
    void DestroySemaphores(VkDevice device, std::vector<VkSemaphore>& semaphores);

    VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format);
}
