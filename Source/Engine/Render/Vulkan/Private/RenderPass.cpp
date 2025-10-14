#include "Engine/Render/Vulkan/RenderPass.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace RenderPassDetails
{
    static VkSubpassDependency2 GetSubpassDependency(const PipelineBarrier& barrier, const bool previous)
    {
        return {
            .sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
            .pNext = nullptr,
            .srcSubpass = previous ? VK_SUBPASS_EXTERNAL : 0,
            .dstSubpass = previous ? 0 : VK_SUBPASS_EXTERNAL,
            .srcStageMask = barrier.srcStage,
            .dstStageMask = barrier.dstStage,
            .srcAccessMask = barrier.srcAccessMask,
            .dstAccessMask = barrier.dstAccessMask, };
    }

    static VkAttachmentReference2 GetAttachmentReference(const size_t attachment, const VkImageLayout layout)
    {
        return { .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2, .pNext = nullptr,
            .attachment = static_cast<uint32_t>(attachment), .layout = layout };
    }

    static VkSubpassDescriptionDepthStencilResolve GetDepthStencilResolve(const VkAttachmentReference2& depthStencilResolveAttachmentReference)
    {
        return {
            .sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE,
            .depthResolveMode = VK_RESOLVE_MODE_MIN_BIT,
            .stencilResolveMode = VK_RESOLVE_MODE_NONE,
            .pDepthStencilResolveAttachment = &depthStencilResolveAttachmentReference };
    }
}

RenderPass::RenderPass(const VkRenderPass aRenderPass, std::vector<VkClearValue> aDefaultClearValues, const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
    , renderPass{ aRenderPass }
    , defaultClearValues{ std::move(aDefaultClearValues) }
{
    Assert(IsValid());
}

RenderPass::~RenderPass()
{
    if (renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(vulkanContext->GetDevice(), renderPass, nullptr);
    }
}

RenderPass::RenderPass(RenderPass&& other) noexcept
    : vulkanContext{ other.vulkanContext }
    , renderPass{ other.renderPass }
    , defaultClearValues{ std::move(other.defaultClearValues) }
{
    other.vulkanContext = nullptr;
    other.renderPass = VK_NULL_HANDLE;
}

RenderPass& RenderPass::operator=(RenderPass&& other) noexcept
{
    if (this != &other)
    {
        std::swap(vulkanContext, other.vulkanContext);
        std::swap(renderPass, other.renderPass);
        std::swap(defaultClearValues, other.defaultClearValues);
    }

    return *this;
}

VkRenderPassBeginInfo RenderPass::GetBeginInfo(const VkFramebuffer framebuffer)
{
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea = vulkanContext->GetSwapchain().GetRect();
    renderPassInfo.clearValueCount = static_cast<uint32_t>(defaultClearValues.size());
    renderPassInfo.pClearValues = defaultClearValues.data();
    
    return renderPassInfo;
}

void RenderPass::Begin(const Frame& frame, const VkFramebuffer framebuffer, std::optional<GpuTimestamp> timestamp /* = std::nullopt*/)
{
    if (timestamp)
    {
        StatsUtils::WriteTimestamp(frame.commandBuffer, frame.queryPools.timestamps, *timestamp);
    }
    
    const VkRenderPassBeginInfo beginInfo = GetBeginInfo(framebuffer);
    vkCmdBeginRenderPass(frame.commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void RenderPass::End(const Frame& frame, std::optional<GpuTimestamp> timestamp /* = std::nullopt*/)
{
    vkCmdEndRenderPass(frame.commandBuffer);
    
    if (timestamp)
    {
        StatsUtils::WriteTimestamp(frame.commandBuffer, frame.queryPools.timestamps, *timestamp);
    }
}

RenderPassBuilder::RenderPassBuilder(const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
{}

RenderPass RenderPassBuilder::Build()
{
    using namespace RenderPassDetails;
    
    // Set samples only for depth and color attachments, resolve attachments were initialized to have 1 sample
    std::ranges::for_each(colorAttachmentReferences, [&](const VkAttachmentReference2& reference) {
        attachments[reference.attachment].samples = sampleCount;
    });

    if (depthStencilAttachmentReference)
    {
        attachments[depthStencilAttachmentReference->attachment].samples = sampleCount;
    }

    // Always have only 1 subpass
    VkSubpassDescription2 subpass = { .sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2 };
    subpass.pNext = depthStencilResolve ? &depthStencilResolve.value() : nullptr;
    subpass.pipelineBindPoint = bindPoint;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentReferences.size());
    subpass.pColorAttachments = colorAttachmentReferences.data();
    subpass.pDepthStencilAttachment = depthStencilAttachmentReference ? &depthStencilAttachmentReference.value() : VK_NULL_HANDLE;
    subpass.pResolveAttachments = resolveAttachmentReferences.data();

    std::vector<VkSubpassDependency2> dependencies;
    dependencies.reserve(previousBarriers.size() + followingBarriers.size());
    
    std::ranges::transform(previousBarriers, std::back_inserter(dependencies), [](const PipelineBarrier& barrier) {
        return GetSubpassDependency(barrier, true); });
    std::ranges::transform(followingBarriers, std::back_inserter(dependencies), [](const PipelineBarrier& barrier) {
        return GetSubpassDependency(barrier, false); });
    
    VkRenderPassCreateInfo2 renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    VkRenderPass renderPass;
    const VkResult result = vkCreateRenderPass2(vulkanContext->GetDevice(), &renderPassInfo, nullptr, &renderPass);
    Assert(result == VK_SUCCESS);

    return { renderPass, std::move(defaultClearValues), *vulkanContext };
}

RenderPassBuilder& RenderPassBuilder::SetBindPoint(VkPipelineBindPoint aBindPoint)
{
    bindPoint = aBindPoint;

    return *this;
}

RenderPassBuilder& RenderPassBuilder::SetMultisampling(const VkSampleCountFlagBits aSampleCount)
{
    sampleCount = aSampleCount;

    return *this;
}

RenderPassBuilder& RenderPassBuilder::AddColorAttachment(const AttachmentDescription& description,
    const std::optional<AttachmentDescription>& resolveDescription /* = std::nullopt */)
{
    AddAttachment(description);
    colorAttachmentReferences.push_back(RenderPassDetails::GetAttachmentReference(attachments.size() - 1, description.actualLayout));

    if (resolveDescription)
    {
        AddAttachment(*resolveDescription);
        resolveAttachmentReferences.push_back(RenderPassDetails::GetAttachmentReference(attachments.size() - 1, resolveDescription->actualLayout));
    }
    
    return *this;
}

RenderPassBuilder& RenderPassBuilder::AddDepthStencilAttachment(const AttachmentDescription& description,
    const std::optional<AttachmentDescription>& resolveDescription /* = std::nullopt */)
{
    Assert(!depthStencilAttachmentReference);
    Assert(description.stencilLoadOp);
    Assert(description.stencilStoreOp);

    AddAttachment(description);
    depthStencilAttachmentReference = RenderPassDetails::GetAttachmentReference(attachments.size() - 1, description.actualLayout);
    
    if (resolveDescription)
    {
        Assert(!depthStencilResolveAttachmentReference);
        Assert(resolveDescription->stencilLoadOp);
        Assert(resolveDescription->stencilStoreOp);
        
        AddAttachment(*resolveDescription);
        depthStencilResolveAttachmentReference = RenderPassDetails::GetAttachmentReference(attachments.size() - 1, resolveDescription->actualLayout);
        depthStencilResolve = RenderPassDetails::GetDepthStencilResolve(*depthStencilResolveAttachmentReference);
    }

    return *this;
}

RenderPassBuilder& RenderPassBuilder::SetPreviousBarriers(std::vector<PipelineBarrier> barriers)
{
    previousBarriers = std::move(barriers);
    
    return *this;
}

RenderPassBuilder& RenderPassBuilder::SetFollowingBarriers(std::vector<PipelineBarrier> barriers)
{
    followingBarriers = std::move(barriers);
    
    return *this;
}

void RenderPassBuilder::AddAttachment(const AttachmentDescription& description)
{
    VkAttachmentDescription2 attachment = { .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2 };

    attachment.format = description.format;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = description.loadOp;
    attachment.storeOp = description.storeOp;
    attachment.stencilLoadOp = description.stencilLoadOp ? description.stencilLoadOp.value() : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = description.stencilStoreOp ? description.stencilStoreOp.value() : VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = description.initialLayout;
    attachment.finalLayout = description.finalLayout;

    attachments.push_back(attachment);
    defaultClearValues.push_back(description.defaultClearValue);
}
