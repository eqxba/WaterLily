#pragma once

#include <volk.h>

#include "Engine/Render/Vulkan/Frame.hpp"
#include "Engine/Render/Vulkan/StatsUtils.hpp"
#include "Engine/Render/Vulkan/Synchronization/SynchronizationUtils.hpp"

class VulkanContext;

class RenderPass
{
public:
    RenderPass() = default;
    RenderPass(VkRenderPass renderPass, std::vector<VkClearValue> defaultClearValues, const VulkanContext& vulkanContext);
    ~RenderPass();

    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;

    RenderPass(RenderPass&& other) noexcept;
    RenderPass& operator=(RenderPass&& other) noexcept;
    
    VkRenderPassBeginInfo GetBeginInfo(VkFramebuffer framebuffer);
    
    void Begin(const Frame& frame, VkFramebuffer framebuffer, std::optional<GpuTimestamp> timestamp = std::nullopt);
    void End(const Frame& frame, std::optional<GpuTimestamp> timestamp = std::nullopt);

    bool IsValid() const
    {
        return renderPass != VK_NULL_HANDLE && !defaultClearValues.empty();
    }
    
    operator VkRenderPass() const
    {
        return renderPass;
    }
    
    const std::vector<VkClearValue>& GetDefaultClearValues() const
    {
        return defaultClearValues;
    }

private:
    const VulkanContext* vulkanContext = nullptr;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    
    std::vector<VkClearValue> defaultClearValues;
};

struct AttachmentDescription
{
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    std::optional<VkAttachmentLoadOp> stencilLoadOp = std::nullopt;
    std::optional<VkAttachmentStoreOp> stencilStoreOp = std::nullopt;
    VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout actualLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkClearValue defaultClearValue = {};
};

class RenderPassBuilder
{
public:
    explicit RenderPassBuilder(const VulkanContext& vulkanContext);

    RenderPass Build();

    RenderPassBuilder& SetBindPoint(VkPipelineBindPoint bindPoint);
    RenderPassBuilder& SetMultisampling(VkSampleCountFlagBits sampleCount);
    RenderPassBuilder& AddColorAttachment(const AttachmentDescription& description,
        const std::optional<AttachmentDescription>& resolveDescription = std::nullopt);
    RenderPassBuilder& AddDepthStencilAttachment(const AttachmentDescription& description,
        const std::optional<AttachmentDescription>& resolveDescription = std::nullopt);
    RenderPassBuilder& SetPreviousBarriers(std::vector<PipelineBarrier> barriers);
    RenderPassBuilder& SetFollowingBarriers(std::vector<PipelineBarrier> barriers);

private:
    void AddAttachment(const AttachmentDescription& description);

    const VulkanContext* vulkanContext = nullptr;

    std::vector<VkAttachmentDescription2> attachments;

    std::vector<VkAttachmentReference2> colorAttachmentReferences;
    std::vector<VkAttachmentReference2> resolveAttachmentReferences;
    std::optional<VkAttachmentReference2> depthStencilAttachmentReference;
    std::optional<VkAttachmentReference2> depthStencilResolveAttachmentReference;
    
    std::optional<VkSubpassDescriptionDepthStencilResolve> depthStencilResolve;
    
    std::vector<VkClearValue> defaultClearValues;
    
    std::vector<PipelineBarrier> previousBarriers;
    std::vector<PipelineBarrier> followingBarriers;

    VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
};
