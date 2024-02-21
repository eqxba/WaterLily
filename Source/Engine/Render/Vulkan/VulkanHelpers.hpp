#pragma once

#include <volk.h>

using DeviceCommands = std::function<void(VkCommandBuffer)>;

enum class CommandBufferType
{
    eOneTime,
    eLongLived
};

// TODO: This has to be a class with RAII
struct CommandBufferSync
{
    std::vector<VkSemaphore> waitSemaphores;
    std::vector<VkPipelineStageFlags> waitStages;
    std::vector<VkSemaphore> signalSemaphores;
    VkFence fence;
};

namespace VulkanHelpers
{
    void SubmitCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, DeviceCommands commands, 
        const CommandBufferSync& sync);

    VkFence CreateFence(VkDevice device, VkFenceCreateFlags flags);
    std::vector<VkFence> CreateFences(VkDevice device, VkFenceCreateFlags flags, size_t count);
    std::vector<VkSemaphore> CreateSemaphores(VkDevice device, size_t count);

    void DestroyFences(VkDevice device, std::vector<VkFence>& fences);
    void DestroySemaphores(VkDevice device, std::vector<VkSemaphore>& semaphores);

    void DestroyCommandBufferSync(VkDevice device, CommandBufferSync& sync);
}
