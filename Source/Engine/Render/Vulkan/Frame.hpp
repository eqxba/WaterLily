#pragma once

#include "Engine/Render/Resources/CommandBufferSync.hpp"
#include "Engine/Render/Resources/Buffer.hpp"

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
    
    VkDescriptorSet GetDescriptorSet() const
    {
        return descriptorSet;
    }
    
    void SetDescriptorSet(VkDescriptorSet aDescriptorSet)
    {
        descriptorSet = aDescriptorSet;
    }
    
private:
    const VulkanContext* vulkanContext;
    
    // Do not have to be recreated, persistent for FrameLoop
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    CommandBufferSync sync;
    
    Buffer uniformBuffer;
    
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};
