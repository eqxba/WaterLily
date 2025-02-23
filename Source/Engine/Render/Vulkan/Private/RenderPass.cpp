#include "Engine/Render/Vulkan/RenderPass.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

RenderPass::RenderPass(const VkRenderPass aRenderPass, const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
    , renderPass{ aRenderPass }
{
    Assert(IsValid());
}

RenderPass::~RenderPass()
{
    if (renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(vulkanContext->GetDevice().GetVkDevice(), renderPass, nullptr);
    }
}

RenderPass::RenderPass(RenderPass&& other) noexcept
    : vulkanContext{ other.vulkanContext }
    , renderPass{ other.renderPass }
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
    }

    return *this;
}

RenderPassBuilder::RenderPassBuilder(const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
{}

RenderPass RenderPassBuilder::Build()
{
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

    // TODO: depencencies
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkRenderPass renderPass;
    const VkResult result = vkCreateRenderPass(vulkanContext->GetDevice().GetVkDevice(), &renderPassInfo, nullptr, &renderPass);
    Assert(result == VK_SUCCESS);

    return { renderPass, *vulkanContext };
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
}