#pragma once

#include <volk.h>

using DeviceCommands = std::function<void(VkCommandBuffer)>;

enum class CommandBufferType
{
    eOneTime,
    eLongLived
};

class CommandBufferSync
{
public:
    CommandBufferSync() = default;
    CommandBufferSync(std::vector<VkSemaphore> aWaitSemaphores, std::vector<VkPipelineStageFlags> aWaitStages, 
        std::vector<VkSemaphore> aSignalSemaphores, VkFence aFence, VkDevice device);
    ~CommandBufferSync();

    CommandBufferSync(const CommandBufferSync&) = delete;
    CommandBufferSync& operator=(const CommandBufferSync&) = delete;

    CommandBufferSync(CommandBufferSync&& other) noexcept;
    CommandBufferSync& operator=(CommandBufferSync&& other) noexcept;

    const std::tuple<const std::vector<VkSemaphore>&, const std::vector<VkPipelineStageFlags>&, 
        const std::vector<VkSemaphore>&, VkFence> AsTuple() const
    {
        return { waitSemaphores, waitStages, signalSemaphores, fence };
    }

    const std::vector<VkSemaphore>& GetWaitSemaphores() const
    {
        return waitSemaphores;
    }

    const std::vector<VkPipelineStageFlags>& GetWaitStages() const
    {
        return waitStages;
    }

    const std::vector<VkSemaphore>& GetSignalSemaphores() const
    {
        return signalSemaphores;
    }

    VkFence GetFence() const
    {
        return fence;
    }

private:
    std::vector<VkSemaphore> waitSemaphores;
    std::vector<VkPipelineStageFlags> waitStages;
    std::vector<VkSemaphore> signalSemaphores;
    VkFence fence = VK_NULL_HANDLE;

    VkDevice device = VK_NULL_HANDLE;
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
}
