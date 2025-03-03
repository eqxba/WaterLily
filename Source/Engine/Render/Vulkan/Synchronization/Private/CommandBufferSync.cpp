#include "Engine/Render/Vulkan/Synchronization/CommandBufferSync.hpp"

#include "Engine/Render/Vulkan/VulkanUtils.hpp"

CommandBufferSync::CommandBufferSync(std::vector<VkSemaphore> aWaitSemaphores,
    std::vector<VkPipelineStageFlags> aWaitStages, std::vector<VkSemaphore> aSignalSemaphores,
    VkFence aFence, VkDevice aDevice)
    : waitSemaphores{ std::move(aWaitSemaphores) }
    , waitStages{ std::move(aWaitStages) }
    , signalSemaphores{ std::move(aSignalSemaphores) }
    , fence{ aFence }
    , device{ aDevice }
{}

CommandBufferSync::~CommandBufferSync()
{
    if (device == VK_NULL_HANDLE)
    {
        return;
    }

    VulkanUtils::DestroySemaphores(device, waitSemaphores);
    VulkanUtils::DestroySemaphores(device, signalSemaphores);

    if (fence != VK_NULL_HANDLE)
    {
        vkDestroyFence(device, fence, nullptr);
        fence = VK_NULL_HANDLE;
    }

    device = VK_NULL_HANDLE;
}

CommandBufferSync::CommandBufferSync(CommandBufferSync&& other) noexcept
    : waitSemaphores{ std::move(other.waitSemaphores) }
    , waitStages{ std::move(other.waitStages) }
    , signalSemaphores{ std::move(other.signalSemaphores) }
    , fence{ other.fence }
    , device{ other.device }
{
    other.waitSemaphores.clear();
    other.waitStages.clear();
    other.signalSemaphores.clear();
    other.fence = VK_NULL_HANDLE;
}

CommandBufferSync& CommandBufferSync::operator=(CommandBufferSync&& other) noexcept
{
    if (this != &other)
    {
        std::swap(waitSemaphores, other.waitSemaphores);
        std::swap(waitStages, other.waitStages);
        std::swap(signalSemaphores, other.signalSemaphores);
        std::swap(fence, other.fence);
        std::swap(device, other.device);
    }
    return *this;
}