#pragma once

#include "Engine/Render/Vulkan/Frame.hpp"

#include <volk.h>

class VulkanContext;
class CommandBufferSync;
class Scene;
class Buffer;
class Swapchain;
class ImageView;
class Image;

using DeviceCommands = std::function<void(VkCommandBuffer)>;

enum class CommandBufferType
{
    eOneTime,
    eLongLived
};

namespace VulkanHelpers
{
    std::vector<VkCommandBuffer> CreateCommandBuffers(VkDevice device, const size_t count, VkCommandPool commandPool);

    void SubmitCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, const DeviceCommands& commands,
        const CommandBufferSync& sync);

    VkFence CreateFence(VkDevice device, VkFenceCreateFlags flags);
    std::vector<VkFence> CreateFences(VkDevice device, VkFenceCreateFlags flags, size_t count);
    void DestroyFences(VkDevice device, std::vector<VkFence>& fences);

    VkSemaphore CreateSemaphore(VkDevice device);
    std::vector<VkSemaphore> CreateSemaphores(VkDevice device, size_t count);
    void DestroySemaphores(VkDevice device, std::vector<VkSemaphore>& semaphores);
    
    VkSampler CreateSampler(VkDevice device, VkPhysicalDeviceProperties properties, uint32_t mipLevelsCount);
    void DestroySampler(VkDevice device, VkSampler sampler);

    std::vector<VkDescriptorSet> CreateDescriptorSets(VkDescriptorPool descriptorPool, VkDescriptorSetLayout layout,
        size_t count, VkDevice device);
    void PopulateDescriptorSet(const VkDescriptorSet set, const Buffer& uniformBuffer, const Scene& scene, VkDevice device);

    std::vector<VkFramebuffer> CreateFramebuffers(const Swapchain& swapchain, const ImageView& colorAttachmentView,
        const ImageView& depthAttachmentView, VkRenderPass renderPass, VkDevice device);
    void DestroyFramebuffers(std::vector<VkFramebuffer>& framebuffers, VkDevice device);

    std::vector<Frame> CreateFrames(const size_t count, const VulkanContext& vulkanContext);

    std::unique_ptr<Image> CreateColorAttachment(VkExtent2D extent, const VulkanContext& vulkanContext);
    std::unique_ptr<Image> CreateDepthAttachment(VkExtent2D extent, const VulkanContext& vulkanContext);

    VkDescriptorPool CreateDescriptorPool(const uint32_t descriptorCount, const uint32_t maxSets, VkDevice device);

    std::tuple<std::unique_ptr<Buffer>, uint32_t> CreateIndirectBuffer(const Scene& scene, const VulkanContext& vulkanContext);

    VkViewport GetViewport(const VkExtent2D extent);
    VkRect2D GetScissor(const VkExtent2D extent);
}
