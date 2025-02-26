#pragma once

#include <volk.h>

#include "Engine/Render/Resources/CommandBufferSync.hpp"

struct Frame
{
    uint32_t index = 0;
    uint32_t swapchainImageIndex = 0;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    CommandBufferSync sync;
};
