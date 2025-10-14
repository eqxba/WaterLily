#include "Engine/Render/Utils/ForwardUtils.hpp"

#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Image/ImageUtils.hpp"

namespace ForwardUtilsDetails
{
    static constexpr VkClearColorValue clearColorValue = { { 0.73f, 0.95f, 1.0f, 1.0f } };
    static constexpr VkClearDepthStencilValue clearDepthStencilValue = { 0.0f, 0 };
}

RenderPass ForwardUtils::CreateRenderPass(VkSampleCountFlagBits sampleCount, const VulkanContext& vulkanContext)
{
    const bool singleSample = sampleCount == VK_SAMPLE_COUNT_1_BIT;
    
    const AttachmentDescription colorAttachmentDescription = {
        .format = vulkanContext.GetSwapchain().GetSurfaceFormat().format,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = singleSample ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = singleSample ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .actualLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .defaultClearValue = { .color = ForwardUtilsDetails::clearColorValue } };
    
    const AttachmentDescription resolveAttachmentDescription = {
        .format = vulkanContext.GetSwapchain().GetSurfaceFormat().format,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .actualLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    
    const auto resolveAttachmentDescriptionOpt = singleSample ? std::nullopt : std::optional(resolveAttachmentDescription);
    
    const AttachmentDescription depthStencilAttachmentDescription = {
        .format = VulkanConfig::depthImageFormat,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .actualLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .defaultClearValue = { .depthStencil = ForwardUtilsDetails::clearDepthStencilValue } };
    
    return RenderPassBuilder(vulkanContext)
        .SetBindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS)
        .SetMultisampling(sampleCount)
        .AddColorAttachment(colorAttachmentDescription, resolveAttachmentDescriptionOpt)
        .AddDepthStencilAttachment(depthStencilAttachmentDescription)
        .SetPreviousBarriers({ Barriers::lateDepthStencilWriteToEarlyAndLateDepthStencilReadWrite, Barriers::colorWriteToColorReadWrite })
        .Build();
}

RenderPass ForwardUtils::CreateFirstOcclusionCullingRenderPass(const VkSampleCountFlagBits sampleCount, const VulkanContext& vulkanContext)
{
    const bool singleSample = sampleCount == VK_SAMPLE_COUNT_1_BIT; // Rendering into swapchain directly
    
    const AttachmentDescription colorAttachmentDescription = {
        .format = vulkanContext.GetSwapchain().GetSurfaceFormat().format,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = singleSample ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .actualLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .defaultClearValue = { .color = ForwardUtilsDetails::clearColorValue } };
    
    const AttachmentDescription depthStencilAttachmentDescription = {
        .format = VulkanConfig::depthImageFormat,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .actualLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .finalLayout = singleSample ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .defaultClearValue = { .depthStencil = ForwardUtilsDetails::clearDepthStencilValue } };
    
    const AttachmentDescription depthStencilResolveAttachmentDescription = {
        .format = VulkanConfig::depthImageFormat,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE, // We're building depth pyramid from it
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .actualLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }; // So we'll need to read from it
    
    const auto depthStencilResolveAttachmentDescriptionOpt = singleSample ? std::nullopt : std::optional(depthStencilResolveAttachmentDescription);
    
    return RenderPassBuilder(vulkanContext)
        .SetBindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS)
        .SetMultisampling(sampleCount)
        .AddColorAttachment(colorAttachmentDescription)
        .AddDepthStencilAttachment(depthStencilAttachmentDescription, depthStencilResolveAttachmentDescriptionOpt)
        .SetPreviousBarriers({ Barriers::lateDepthStencilWriteToEarlyAndLateDepthStencilReadWrite, Barriers::colorWriteToColorReadWrite })
        .Build();
}

RenderPass ForwardUtils::CreateSecondOcclusionCullingRenderPass(const VkSampleCountFlagBits sampleCount, const VulkanContext& vulkanContext)
{
    const bool singleSample = sampleCount == VK_SAMPLE_COUNT_1_BIT;
    
    const AttachmentDescription colorAttachmentDescription = {
        .format = vulkanContext.GetSwapchain().GetSurfaceFormat().format,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD, // Load in the 2nd pass
        .storeOp = singleSample ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .actualLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    
    const AttachmentDescription resolveAttachmentDescription = {
        .format = vulkanContext.GetSwapchain().GetSurfaceFormat().format,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .actualLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    
    const auto resolveAttachmentDescriptionOpt = singleSample ? std::nullopt : std::optional(resolveAttachmentDescription);
    
    const AttachmentDescription depthStencilAttachmentDescription = {
        .format = VulkanConfig::depthImageFormat,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = singleSample ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .actualLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
    
    return RenderPassBuilder(vulkanContext)
        .SetBindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS)
        .SetMultisampling(sampleCount)
        .AddColorAttachment(colorAttachmentDescription, resolveAttachmentDescriptionOpt)
        .AddDepthStencilAttachment(depthStencilAttachmentDescription)
        .SetPreviousBarriers({ Barriers::lateDepthStencilWriteToEarlyAndLateDepthStencilReadWrite, Barriers::colorWriteToColorReadWrite })
        .Build();
}

RenderTarget ForwardUtils::CreateColorTarget(const VkSampleCountFlagBits sampleCount, const VulkanContext& vulkanContext)
{
    const Swapchain& swapchain = vulkanContext.GetSwapchain();
    const VkExtent2D swapchainExtent = swapchain.GetExtent();

    ImageDescription colorTargetDescription = {
        .extent = { swapchainExtent.width, swapchainExtent.height, 1 },
        .mipLevelsCount = 1,
        .samples = sampleCount,
        .format = swapchain.GetSurfaceFormat().format,
        .usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
    
    auto renderTarget = RenderTarget(std::move(colorTargetDescription), VK_IMAGE_ASPECT_COLOR_BIT, vulkanContext);
    
    vulkanContext.GetDevice().ExecuteOneTimeCommandBuffer([&](VkCommandBuffer cmd) {
        ImageUtils::TransitionLayout(cmd, renderTarget, LayoutTransitions::undefinedToColorAttachmentOptimal, Barriers::noneToColorWrite);
    });
    
    return renderTarget;
}

RenderTarget ForwardUtils::CreateDepthTarget(const VkSampleCountFlagBits sampleCount, const VulkanContext& vulkanContext)
{
    const VkExtent2D swapchainExtent = vulkanContext.GetSwapchain().GetExtent();
    
    ImageDescription depthTargetDescription = {
        .extent = { swapchainExtent.width, swapchainExtent.height, 1 },
        .mipLevelsCount = 1,
        .samples = sampleCount,
        .format = VulkanConfig::depthImageFormat,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | (sampleCount == VK_SAMPLE_COUNT_1_BIT
            ? VK_IMAGE_USAGE_SAMPLED_BIT : static_cast<VkBufferUsageFlags>(0)),
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
    
    auto renderTarget = RenderTarget(std::move(depthTargetDescription), VK_IMAGE_ASPECT_DEPTH_BIT, vulkanContext);
    
    vulkanContext.GetDevice().ExecuteOneTimeCommandBuffer([&](VkCommandBuffer cmd) {
        ImageUtils::TransitionLayout(cmd, renderTarget, LayoutTransitions::undefinedToDepthStencilAttachmentOptimal, Barriers::noneToEarlyAndLateDepthStencilWrite);
    });
    
    return renderTarget;
}

RenderTarget ForwardUtils::CreateDepthResolveTarget(const VulkanContext& vulkanContext)
{
    const VkExtent2D swapchainExtent = vulkanContext.GetSwapchain().GetExtent();
    
    ImageDescription depthResolveTargetDescription = {
        .extent = { swapchainExtent.width, swapchainExtent.height, 1 },
        .mipLevelsCount = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .format = VulkanConfig::depthImageFormat,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
    
    auto renderTarget = RenderTarget(std::move(depthResolveTargetDescription), VK_IMAGE_ASPECT_DEPTH_BIT, vulkanContext);
    
    vulkanContext.GetDevice().ExecuteOneTimeCommandBuffer([&](VkCommandBuffer cmd) {
        ImageUtils::TransitionLayout(cmd, renderTarget, LayoutTransitions::undefinedToShaderReadOnlyOptimal, Barriers::noneToResolve);
    });
    
    return renderTarget;
}
