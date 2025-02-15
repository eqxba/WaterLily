#pragma once

#include "Engine/Render/Resources/CommandBufferSync.hpp"
#include "Engine/Render/Resources/Buffer.hpp"
#include "Engine/Render/Resources/Descriptor.hpp"

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
    
    Buffer& GetUniformBuffer()
    {
        return uniformBuffer;
    }
    
    const std::vector<Descriptor>& GetDescriptors() const
    {
        return descriptors;
    }

private:
    const VulkanContext* vulkanContext;
    
    // Do not have to be recreated, persistent for FrameLoop
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    CommandBufferSync sync;
    
    Buffer uniformBuffer;

    std::vector<Descriptor> descriptors;
};
