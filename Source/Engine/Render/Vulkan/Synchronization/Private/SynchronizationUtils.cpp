#include "Engine/Render/Vulkan/Synchronization/SynchronizationUtils.hpp"

void SynchronizationUtils::SetMemoryBarrier(const VkCommandBuffer commandBuffer, const PipelineBarrier barrier)
{
    const VkMemoryBarrier memoryBarrier = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .srcAccessMask = barrier.srcAccessMask,
        .dstAccessMask = barrier.dstAccessMask, };

    vkCmdPipelineBarrier(commandBuffer, barrier.srcStage, barrier.dstStage, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
}