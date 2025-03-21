#pragma once

#include <volk.h>

struct PipelineBarrier
{
    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkAccessFlags srcAccessMask = VK_ACCESS_NONE;
    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    VkAccessFlags dstAccessMask = VK_ACCESS_NONE;
};

namespace Barriers
{
    static constexpr PipelineBarrier transferWriteToComputeRead = {
        .srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT };
}

namespace SynchronizationUtils
{
    // TODO: Buffer barriers instead
    void SetMemoryBarrier(VkCommandBuffer commandBuffer, PipelineBarrier barrier);
}