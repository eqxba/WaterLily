#pragma once

#include <volk.h>

#include "Engine/Render/Resources/Descriptor.hpp"

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

    std::vector<VkFramebuffer> CreateFramebuffers(const Swapchain& swapchain, const ImageView& colorAttachmentView,
        const ImageView& depthAttachmentView, VkRenderPass renderPass, VkDevice device);
    void DestroyFramebuffers(std::vector<VkFramebuffer>& framebuffers, VkDevice device);

    std::unique_ptr<Image> CreateColorAttachment(VkExtent2D extent, const VulkanContext& vulkanContext);
    std::unique_ptr<Image> CreateDepthAttachment(VkExtent2D extent, const VulkanContext& vulkanContext);

    std::tuple<std::unique_ptr<Buffer>, uint32_t> CreateIndirectBuffer(const Scene& scene, const VulkanContext& vulkanContext);

    VkViewport GetViewport(const VkExtent2D extent);
    VkRect2D GetScissor(const VkExtent2D extent);

    std::vector<VkDescriptorSetLayout> GetUniqueVkDescriptorSetLayouts(const std::vector<Descriptor>& descriptors);
    std::vector<VkDescriptorSet> GetVkDescriptorSets(const std::vector<Descriptor>& descriptors);
}