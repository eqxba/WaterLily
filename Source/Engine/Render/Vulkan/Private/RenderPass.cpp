#include "Engine/Render/Vulkan/RenderPass.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace RenderPassDetails
{
    static VkSubpassDependency GetSubpassDependency(const PipelineBarrier& barrier, const bool previous)
    {
        return {
            .srcSubpass = previous ? VK_SUBPASS_EXTERNAL : 0,
            .dstSubpass = previous ? 0 : VK_SUBPASS_EXTERNAL,
            .srcStageMask = barrier.srcStage,
            .dstStageMask = barrier.dstStage,
            .srcAccessMask = barrier.srcAccessMask,
            .dstAccessMask = barrier.dstAccessMask, };
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

RenderPassBuilder::RenderPassBuilder(const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
{}

RenderPass RenderPassBuilder::Build()
{
    using namespace RenderPassDetails;
    
    // Set samples only for depth and color attachments, resolve attachments were initialized to have 1 sample
    std::ranges::for_each(colorAttachmentReferences, [&](const VkAttachmentReference& reference) {
        attachments[reference.attachment].samples = sampleCount;
    });

    if (depthStencilAttachmentReference)
    {
        attachments[depthStencilAttachmentReference->attachment].samples = sampleCount;
    }

    // Always have only 1 subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = bindPoint;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentReferences.size());
    subpass.pColorAttachments = colorAttachmentReferences.data();
    subpass.pDepthStencilAttachment = depthStencilAttachmentReference ? &depthStencilAttachmentReference.value() : VK_NULL_HANDLE;
    subpass.pResolveAttachments = resolveAttachmentReferences.data();

    std::vector<VkSubpassDependency> dependencies;
    dependencies.reserve(previousBarriers.size() + followingBarriers.size());
    
    std::ranges::transform(previousBarriers, std::back_inserter(dependencies), [](const PipelineBarrier& barrier) {
        return GetSubpassDependency(barrier, true); });
    std::ranges::transform(followingBarriers, std::back_inserter(dependencies), [](const PipelineBarrier& barrier) {
        return GetSubpassDependency(barrier, false); });

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    VkRenderPass renderPass;
    const VkResult result = vkCreateRenderPass(vulkanContext->GetDevice(), &renderPassInfo, nullptr, &renderPass);
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

RenderPassBuilder& RenderPassBuilder::AddColorAttachment(const AttachmentDescription& description)
{
    AddAttachment(description);
    colorAttachmentReferences.emplace_back(static_cast<uint32_t>(attachments.size() - 1), description.actualLayout);

    return *this;
}

RenderPassBuilder& RenderPassBuilder::AddColorAndResolveAttachments(const AttachmentDescription& colorDescription, 
    const AttachmentDescription& resolveDescription)
{
    AddColorAttachment(colorDescription);

    AddAttachment(resolveDescription);
    resolveAttachmentReferences.emplace_back(static_cast<uint32_t>(attachments.size() - 1), resolveDescription.actualLayout);

    return *this;
}

RenderPassBuilder& RenderPassBuilder::AddDepthStencilAttachment(const AttachmentDescription& description)
{
    Assert(!depthStencilAttachmentReference.has_value());
    Assert(description.stencilLoadOp.has_value());
    Assert(description.stencilStoreOp.has_value());

    AddAttachment(description);
    depthStencilAttachmentReference = { static_cast<uint32_t>(attachments.size() - 1), description.actualLayout };

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
    VkAttachmentDescription attachment{};

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
