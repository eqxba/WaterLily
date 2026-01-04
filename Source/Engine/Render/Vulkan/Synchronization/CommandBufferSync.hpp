#pragma once

#include <volk.h>

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

    std::tuple<std::vector<VkSemaphore>&, std::vector<VkPipelineStageFlags>&, std::vector<VkSemaphore>&, VkFence> AsTuple()
    {
        return { waitSemaphores, waitStages, signalSemaphores, fence };
    }

    std::tuple<const std::vector<VkSemaphore>&, const std::vector<VkPipelineStageFlags>&, const std::vector<VkSemaphore>&, VkFence> AsTuple() const
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