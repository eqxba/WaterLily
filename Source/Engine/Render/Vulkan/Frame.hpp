#pragma once

#include <volk.h>

#include "Engine/Render/Vulkan/Synchronization/CommandBufferSync.hpp"

struct RenderStats
{
    uint64_t triangleCount = 0; // VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT
};

struct Frame
{
    uint32_t index = 0;
    uint32_t swapchainImageIndex = 0;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    CommandBufferSync sync;

    RenderStats stats;
};
