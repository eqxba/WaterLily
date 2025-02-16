#pragma once

#include <volk.h>

#include "Engine/Render/Resources/Descriptor.hpp"
#include "Engine/Render/Shaders/ShaderModule.hpp"

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

    std::vector<VkPipelineShaderStageCreateInfo> GetShaderStageCreateInfos(const std::vector<ShaderModule>& shaders);

    VkPipelineLayout CreatePipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
        const std::vector<VkPushConstantRange>& pushConstantRanges, VkDevice device);

    template <typename... Args>
    std::vector<VkDescriptorSetLayout> GetUniqueVkDescriptorSetLayouts(const Args&... descriptorVectors)
    {
        std::vector<VkDescriptorSetLayout> layouts;
        std::unordered_set<VkDescriptorSetLayout> uniqueLayouts;

        const auto insertLayout = [&](const Descriptor& descriptor) {
            if (uniqueLayouts.insert(descriptor.layout).second)
            {
                layouts.push_back(descriptor.layout);
            }
        };

        const auto insertLayouts = [&](const auto& descriptors) { std::ranges::for_each(descriptors, insertLayout); };

        (insertLayouts(descriptorVectors), ...);

        return layouts;
    }

    template <typename... Args>
    std::vector<VkDescriptorSet> GetVkDescriptorSets(const Args&... descriptorVectors)
    {
        std::vector<VkDescriptorSet> sets;

        auto insertSets = [&sets](const auto& descriptors) {
            std::ranges::transform(descriptors, std::back_inserter(sets), [](const Descriptor& descriptor) {
                return descriptor.set;
            });
        };

        (insertSets(descriptorVectors), ...);

        return sets;
    }
}