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
    
    constexpr PipelineBarrier transferReadToComputeRead = {
        .srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT };

    constexpr PipelineBarrier transferReadToTransferWrite = {
        .srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT };

    constexpr PipelineBarrier transferWriteToTransferRead = {
        .srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT };

    constexpr PipelineBarrier transferWriteToComputeRead = {
        .srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT };

    constexpr PipelineBarrier transferWriteToComputeReadWrite = {
        .srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT };

    constexpr PipelineBarrier transferWriteToVertexInputRead = {
        .srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        .dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT };

    constexpr PipelineBarrier transferWriteToColorReadWrite = {
        .srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT };

    constexpr PipelineBarrier transferWriteToFragmentRead = {
        .srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT };

    constexpr PipelineBarrier computeReadToComputeWrite = {
        .srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT };

    constexpr PipelineBarrier computeReadToTransferRead = {
        .srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT };

    constexpr PipelineBarrier computeWriteToComputeRead = {
        .srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT };

    constexpr PipelineBarrier computeWriteToTransferRead = {
        .srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT };

    constexpr PipelineBarrier computeWriteToIndirectCommandRead = {
        .srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
        .dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT };

    constexpr PipelineBarrier computeWriteToTaskRead = {
        .srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT };

    constexpr PipelineBarrier lateDepthStencilWriteToComputeRead = {
        .srcStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT };

    constexpr PipelineBarrier lateDepthStencilWriteToEarlyAndLateDepthStencilReadWrite = {
        .srcStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT };
        
    constexpr PipelineBarrier colorReadWriteToTransferWrite = {
        .srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT };

    constexpr PipelineBarrier colorWriteToColorReadWrite = {
        .srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT };

    constexpr PipelineBarrier colorWriteToComputeRead = {
        .srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT };

    constexpr PipelineBarrier resolveToComputeRead = colorWriteToComputeRead;

    constexpr PipelineBarrier noneToTransferWrite = {
        .srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        .srcAccessMask = VK_ACCESS_NONE,
        .dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT };

    constexpr PipelineBarrier noneToComputeWrite = {
        .srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        .srcAccessMask = VK_ACCESS_NONE,
        .dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT };

    constexpr PipelineBarrier noneToColorWrite = {
        .srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        .srcAccessMask = VK_ACCESS_NONE,
        .dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT };

    constexpr PipelineBarrier noneToResolve = noneToColorWrite;

    constexpr PipelineBarrier noneToEarlyAndLateDepthStencilWrite = {
        .srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        .srcAccessMask = VK_ACCESS_NONE,
        .dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT };

    constexpr PipelineBarrier indirectCommandReadToTransferWrite = {
        .srcStage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
        .srcAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
        .dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT };
}

namespace SynchronizationUtils
{
    // TODO: Buffer barriers instead
    void SetMemoryBarrier(VkCommandBuffer commandBuffer, PipelineBarrier barrier);
}

inline constexpr PipelineBarrier operator|(const PipelineBarrier& lhs, const PipelineBarrier& rhs)
{
    return {
        .srcStage = lhs.srcStage | rhs.srcStage,
        .srcAccessMask = lhs.srcAccessMask | rhs.srcAccessMask,
        .dstStage = lhs.dstStage | rhs.dstStage,
        .dstAccessMask = lhs.dstAccessMask | rhs.dstAccessMask };
}
