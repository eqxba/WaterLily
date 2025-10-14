#pragma once

#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/Image/RenderTarget.hpp"

class Swapchain;
class VulkanContext;

namespace ForwardUtils
{
    RenderPass CreateRenderPass(VkSampleCountFlagBits sampleCount, const VulkanContext& vulkanContext);
    RenderPass CreateFirstOcclusionCullingRenderPass(VkSampleCountFlagBits sampleCount, const VulkanContext& vulkanContext);
    RenderPass CreateSecondOcclusionCullingRenderPass(VkSampleCountFlagBits sampleCount, const VulkanContext& vulkanContext);

    RenderTarget CreateColorTarget(VkSampleCountFlagBits sampleCount, const VulkanContext& vulkanContext);
    RenderTarget CreateDepthTarget(VkSampleCountFlagBits sampleCount, const VulkanContext& vulkanContext);
    RenderTarget CreateDepthResolveTarget(const VulkanContext& vulkanContext);
}
