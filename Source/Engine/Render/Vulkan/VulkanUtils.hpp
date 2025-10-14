#pragma once

#include <volk.h>

#include "Engine/Render/Vulkan/Pipelines/Pipeline.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderModule.hpp"
#include "Engine/Render/Vulkan/DescriptorSets/DescriptorSetLayout.hpp"
#include "Engine/Render/Vulkan/DescriptorSets/DescriptorSetReflection.hpp"

class VulkanContext;
class RenderPass;
class CommandBufferSync;
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

namespace VulkanUtils
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

    VkFramebuffer CreateFrameBuffer(const RenderPass& renderPass, VkExtent2D extent,
        std::span<const VkImageView> attachments, const VulkanContext& vulkanContext);
    void DestroyFramebuffers(std::vector<VkFramebuffer>& framebuffers, const VulkanContext& vulkanContext);

    // Helper version that inserts swapchain attachments one by be into base attachments array instead of VK_NULL_HANDLE one
    std::vector<VkFramebuffer> CreateFramebuffers(const RenderPass& renderPass, const Swapchain& swapchain, std::vector<VkImageView>& attachments,
        const VulkanContext& vulkanContext);

    VkViewport GetViewport(float width, float height);
    VkRect2D GetScissor(VkExtent2D extent);

    VkPipelineLayout CreatePipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
        const std::vector<VkPushConstantRange>& pushConstantRanges, VkDevice device);
    VkPipelineLayout CreatePipelineLayout(const std::vector<DescriptorSetLayout>& descriptorSetLayouts,
        const std::vector<VkPushConstantRange>& pushConstantRanges, VkDevice device);

    const VkDescriptorSetLayoutBinding& GetBinding(const std::vector<VkDescriptorSetLayoutBinding>& bindings, uint32_t index);

    std::vector<DescriptorSetLayout> CreateDescriptorSetLayouts(const std::vector<DescriptorSetReflection>& reflections,
        const VulkanContext& vulkanContext);

    VkRenderPassBeginInfo GetRenderPassBeginInfo(VkRenderPass renderPass, VkFramebuffer framebuffer, VkRect2D renderArea,
        std::span<const VkClearValue> clearValues);

    void SetDefaultViewportAndScissor(VkCommandBuffer commandBuffer, const Swapchain& swapchain);
}

template <>
struct std::hash<VkDescriptorSetLayoutBinding>
{
    std::size_t operator()(const VkDescriptorSetLayoutBinding& binding) const noexcept
    {
        const std::size_t h1 = std::hash<uint32_t>{}(binding.binding);
        const std::size_t h2 = std::hash<uint32_t>{}(binding.descriptorType);
        const std::size_t h3 = std::hash<uint32_t>{}(binding.descriptorCount);
        const std::size_t h4 = std::hash<uint32_t>{}(binding.stageFlags);
        const std::size_t h5 = std::hash<const void*>{}(binding.pImmutableSamplers);

        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4);
    }
};
