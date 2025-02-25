#pragma once

#include "Engine/Render/Resources/CommandBufferSync.hpp"

class VulkanContext;

class Frame
{
public:
    Frame() = default;
    Frame(const VulkanContext* vulkanContext);
    ~Frame();

    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;
    
    Frame(Frame&& other) noexcept;
    Frame& operator=(Frame&& other) noexcept;
    
    VkCommandBuffer GetCommandBuffer() const
    {
        return commandBuffer;
    }
    
    const CommandBufferSync& GetSync() const
    {
        return sync;
    }

private:
    const VulkanContext* vulkanContext;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    CommandBufferSync sync;
};
