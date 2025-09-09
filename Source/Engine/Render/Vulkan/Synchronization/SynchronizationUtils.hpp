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
    constexpr PipelineBarrier fullBarrier = {
        .srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT,
        .dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT };

    constexpr PipelineBarrier transferWriteToComputeRead = {
        .srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT };

    constexpr PipelineBarrier transferWriteToVertexInputRead = {
        .srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        .dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT
    };
}

namespace SynchronizationUtils
{
    // TODO: Buffer barriers instead
    void SetMemoryBarrier(VkCommandBuffer commandBuffer, PipelineBarrier barrier);
}
