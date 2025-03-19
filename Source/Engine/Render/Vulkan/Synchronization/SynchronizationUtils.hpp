#pragma once

#include <volk.h>

struct PipelineBarrier
{
    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkAccessFlags srcAccessMask = VK_ACCESS_NONE;
    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    VkAccessFlags dstAccessMask = VK_ACCESS_NONE;
};

namespace SynchronizationUtils
{
    // TODO: Buffer barriers instead
    void SetMemoryBarrier(VkCommandBuffer commandBuffer, PipelineBarrier barrier);
}