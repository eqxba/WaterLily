#include "Engine/Render/RenderStages/PrimitiveCullStage.hpp"

#include "Shaders/Common.h"
#include "Engine/Render/RenderOptions.hpp"
#include "Engine/Render/Vulkan/Image/ImageUtils.hpp"
#include "Engine/Render/Vulkan/Pipelines/PipelineUtils.hpp"
#include "Engine/Render/Vulkan/Pipelines/ComputePipelineBuilder.hpp"
#include "Engine/Render/Vulkan/Synchronization/SynchronizationUtils.hpp"

#include <bit>

namespace PrimitiveCullStageDetails
{
    static constexpr std::string_view cullShaderPath = "~/Shaders/Culling/PrimitiveCull.comp";
    static constexpr std::string_view depthPyramidShaderPath = "~/Shaders/Culling/DepthPyramid.comp";

    // TODO: Move from here and from other places to Sync utils
    static constexpr PipelineBarrier beforeClearCommandCountBarrier = {
        .srcStage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
        .srcAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
        .dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT };

    static constexpr PipelineBarrier afterClearCommandCountBarrier = {
        .srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT };

    static constexpr PipelineBarrier afterCullBarrier = {
        .srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
        .dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT };

    static constexpr PipelineBarrier meshAfterCullBarrier = {
        .srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT,
        .dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_SHADER_READ_BIT };

    constexpr PipelineBarrier beforePyramidBuildBarrier = {
        .srcStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT };

    constexpr PipelineBarrier beforePyramidBuildMsDepthBarrier = {
        .srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT };
}

PrimitiveCullStage::PrimitiveCullStage(const VulkanContext& aVulkanContext, RenderContext& aRenderContext)
    : RenderStage{ aVulkanContext, aRenderContext }
{
    AddPipeline(firstPassPipeline, [&]() { return BuildFirstPassPipeline(); });
    AddPipeline(depthPyramidPipeline, [&]() { return BuildDepthPyramidPipeline(); });
    AddPipeline(secondPassPipeline, [&]() { return BuildSecondPassPipeline(); });
}

PrimitiveCullStage::~PrimitiveCullStage() = default;

void PrimitiveCullStage::CreateRenderTargetDependentResources()
{
    CreateDepthPyramidRenderTargetAndSampler();
    BuildDepthPyramidDescriptors();
}

void PrimitiveCullStage::DestroyRenderTargetDependentResources()
{
    depthPyramidDescriptor = VK_NULL_HANDLE;
    depthPyramidReductionDescriptors.clear();
    depthPyramidSampler = {};
    depthPyramidRenderTarget = {};
}

void PrimitiveCullStage::OnSceneOpen(const Scene& scene)
{
    BuildFirstPassDescriptors();
    BuildSecondPassDescriptors();
}

void PrimitiveCullStage::OnSceneClose()
{
    firstPassDescriptors.clear();
    secondPassDescriptors.clear();
}

void PrimitiveCullStage::Execute(const Frame& frame)
{
    Assert(false); // Use dedicated Execute methods (ExecuteFirstPass, BuildDepthPyramid, ExecuteSecondPass);
}

void PrimitiveCullStage::RebuildDescriptors()
{
    firstPassDescriptors.clear();
    depthPyramidReductionDescriptors.clear();
    depthPyramidDescriptor = VK_NULL_HANDLE;
    secondPassDescriptors.clear();
    
    BuildFirstPassDescriptors();
    BuildDepthPyramidDescriptors();
    BuildSecondPassDescriptors();
}

void PrimitiveCullStage::ExecuteFirstPass(const Frame& frame)
{
    using namespace SynchronizationUtils;
    using namespace PrimitiveCullStageDetails;
    using namespace PipelineUtils;

    const VkCommandBuffer cmd = frame.commandBuffer;

    SetMemoryBarrier(cmd, beforeClearCommandCountBarrier);
    vkCmdFillBuffer(cmd, renderContext->commandCountBuffer, 0, sizeof(uint32_t), 0);
    SetMemoryBarrier(cmd, afterClearCommandCountBarrier);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, firstPassPipeline);

    PushConstants(cmd, firstPassPipeline, "globals", renderContext->globals);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, firstPassPipeline.GetLayout(), 0,
        static_cast<uint32_t>(firstPassDescriptors.size()), firstPassDescriptors.data(), 0, nullptr);
    
    vkCmdDispatch(cmd, GroupCount(renderContext->globals.drawCount, gpu::primitiveCullWgSize), 1, 1);

    SetMemoryBarrier(cmd, RenderOptions::Get().GetGraphicsPipelineType() == GraphicsPipelineType::eMesh ? meshAfterCullBarrier : afterCullBarrier);
}

void PrimitiveCullStage::BuildDepthPyramid(const Frame& frame)
{
    using namespace ImageUtils;
    using namespace PipelineUtils;
    using namespace SynchronizationUtils;
    using namespace PrimitiveCullStageDetails;
    
    const VkCommandBuffer cmd = frame.commandBuffer;
    
    SetMemoryBarrier(cmd, RenderOptions::Get().GetMsaaSampleCount() == 1 ? beforePyramidBuildBarrier : beforePyramidBuildMsDepthBarrier);
    
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, depthPyramidPipeline);
    
    const VkExtent3D pyramidExtent = depthPyramidRenderTarget.image.GetDescription().extent;
    
    for (uint32_t targetMip = 0; targetMip < depthPyramidRenderTarget.image.GetDescription().mipLevelsCount; ++targetMip)
    {
        TransitionLayout(cmd, depthPyramidRenderTarget, LayoutTransitions::shaderReadOnlyOptimalToGeneral, {
            .srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT }, targetMip);
        
        const uint32_t dstWidth = std::max(1u, pyramidExtent.width >> targetMip);
        const uint32_t dstHeight = std::max(1u, pyramidExtent.height >> targetMip);
        
        PushConstants(cmd, depthPyramidPipeline, "dstSize", glm::vec2(dstWidth, dstHeight));
        
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, depthPyramidPipeline.GetLayout(), 0,
            1, &depthPyramidReductionDescriptors[targetMip], 0, nullptr);
        
        vkCmdDispatch(cmd, GroupCount(dstWidth, gpu::depthPyramidWgSize), GroupCount(dstHeight, gpu::depthPyramidWgSize), 1);
        
        TransitionLayout(cmd, depthPyramidRenderTarget, LayoutTransitions::generalToShaderReadOnlyOptimal, {
            .srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, .dstAccessMask = VK_ACCESS_SHADER_READ_BIT }, targetMip);
    }
}

void PrimitiveCullStage::VisualizeDepth(const Frame& frame)
{
    using namespace ImageUtils;
    
    const RenderTarget& targetRt = vulkanContext->GetSwapchain().GetRenderTargets()[frame.swapchainImageIndex];
    
    TransitionLayout(frame.commandBuffer, depthPyramidRenderTarget, LayoutTransitions::shaderReadOnlyOptimalToSrcOptimal, {
        .srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT, .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT });
    
    TransitionLayout(frame.commandBuffer, targetRt, LayoutTransitions::colorAttachmentOptimalToDstOptimal, {
        .srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT, .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT });

    BlitImageToImage(frame.commandBuffer, depthPyramidRenderTarget, targetRt, RenderOptions::Get().GetDepthMipToVisualize(),
        0, VK_FILTER_NEAREST);
    
    TransitionLayout(frame.commandBuffer, depthPyramidRenderTarget, LayoutTransitions::srcOptimalToShaderReadOnlyOptimal, {
        .srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT, .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, .dstAccessMask = VK_ACCESS_SHADER_READ_BIT });
    
    TransitionLayout(frame.commandBuffer, targetRt, LayoutTransitions::dstOptimalToColorAttachmentOptimal, {
        .srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT, .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT });
}

void PrimitiveCullStage::ExecuteSecondPass(const Frame& frame)
{
    using namespace SynchronizationUtils;
    using namespace PrimitiveCullStageDetails;
    using namespace PipelineUtils;

    const VkCommandBuffer cmd = frame.commandBuffer;

    SetMemoryBarrier(cmd, beforeClearCommandCountBarrier);
    vkCmdFillBuffer(cmd, renderContext->commandCountBuffer, 0, sizeof(uint32_t), 0);
    SetMemoryBarrier(cmd, afterClearCommandCountBarrier);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, secondPassPipeline);

    PushConstants(cmd, secondPassPipeline, "globals", renderContext->globals);
    
    std::vector<VkDescriptorSet> descriptors = secondPassDescriptors;
    descriptors.push_back(depthPyramidDescriptor);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, secondPassPipeline.GetLayout(), 0,
        static_cast<uint32_t>(descriptors.size()), descriptors.data(), 0, nullptr);

    const auto groupCountX = static_cast<uint32_t>(std::ceil(static_cast<float>(renderContext->globals.drawCount) /
        static_cast<float>(gpu::primitiveCullWgSize)));

    vkCmdDispatch(cmd, groupCountX, 1, 1);

    SetMemoryBarrier(cmd, RenderOptions::Get().GetGraphicsPipelineType() == GraphicsPipelineType::eMesh ? meshAfterCullBarrier : afterCullBarrier);
}

Pipeline PrimitiveCullStage::BuildFirstPassPipeline() const
{
    std::vector runtimeDefines = { gpu::defines::meshPipeline, gpu::defines::visualizeLods, gpu::defines::drawIndirectCount };
    std::vector<ShaderDefine> defines = { { "FIRST_PASS", 1 } };
    
    ShaderModule shader = GetShader(PrimitiveCullStageDetails::cullShaderPath, VK_SHADER_STAGE_COMPUTE_BIT, runtimeDefines, defines);
    
    return ComputePipelineBuilder(*vulkanContext)
        .SetShaderModule(shader)
        .SetSpecializationConstants({ { "FIRST_PASS", VK_TRUE } })
        .Build();
}

void PrimitiveCullStage::BuildFirstPassDescriptors()
{
    Assert(firstPassDescriptors.empty());
    
    const bool meshPipeline = RenderOptions::Get().GetGraphicsPipelineType() == GraphicsPipelineType::eMesh;
    
    ReflectiveDescriptorSetBuilder builder = vulkanContext->GetDescriptorSetsManager()
        .GetReflectiveDescriptorSetBuilder(firstPassPipeline, DescriptorScope::eSceneRenderer)
        .Bind("Primitives", renderContext->primitiveBuffer)
        .Bind("Draws", renderContext->drawBuffer)
        .Bind("DrawsVisibility", renderContext->drawsVisibilityBuffer)
        .Bind("CommandCount", renderContext->commandCountBuffer)
        .Bind(meshPipeline ? "TaskCommands" : "IndirectCommands", renderContext->commandBuffer)
        .Bind("Primitives", renderContext->primitiveBuffer);
    
    if (RenderOptions::Get().GetVisualizeLods())
    {
        builder.Bind("DrawsDebugData", renderContext->drawsDebugDataBuffer);
    }
    
    firstPassDescriptors = builder.Build();
}

void PrimitiveCullStage::CreateDepthPyramidRenderTargetAndSampler()
{
    const VkExtent2D swapchainExtent = vulkanContext->GetSwapchain().GetExtent();
    const VkExtent3D extent = { std::bit_floor(swapchainExtent.width - 1), std::bit_floor(swapchainExtent.height - 1), 1 };
    
    const uint32_t mipLevelsCount = ImageUtils::MipLevelsCount(extent);
    Assert(mipLevelsCount >= 1);
    
    RenderOptions::Get().SetMaxDepthMipToVisualize(mipLevelsCount - 1);
    
    ImageDescription pyramidImageDescription = {
        .extent = extent,
        .mipLevelsCount = mipLevelsCount,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .format = VK_FORMAT_R32_SFLOAT,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
    
    depthPyramidRenderTarget = RenderTarget(std::move(pyramidImageDescription), VK_IMAGE_ASPECT_COLOR_BIT, *vulkanContext);
    
    vulkanContext->GetDevice().ExecuteOneTimeCommandBuffer([&](VkCommandBuffer cmd) {
        ImageUtils::TransitionLayout(cmd, depthPyramidRenderTarget, LayoutTransitions::undefinedToShaderReadOnlyOptimal, {
            .srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, .srcAccessMask = VK_ACCESS_NONE,
            .dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, .dstAccessMask = VK_ACCESS_SHADER_READ_BIT });
    });
    
    // TODO: Support on shader side and simplify sampling there
    const bool samplerFilterMinmaxSupported = vulkanContext->GetDevice().GetProperties().samplerFilterMinmaxSupported;
    
    SamplerDescription depthPyramidSamplerDescription = {
        .filter = samplerFilterMinmaxSupported ? VK_FILTER_LINEAR : VK_FILTER_NEAREST,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .reductionMode = samplerFilterMinmaxSupported ? VK_SAMPLER_REDUCTION_MODE_MIN : VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE,
        .maxLod = static_cast<float>(mipLevelsCount)
    };
    
    depthPyramidSampler = Sampler(std::move(depthPyramidSamplerDescription), *vulkanContext);
}

Pipeline PrimitiveCullStage::BuildDepthPyramidPipeline() const
{
    ShaderModule shader = GetShader(PrimitiveCullStageDetails::depthPyramidShaderPath, VK_SHADER_STAGE_COMPUTE_BIT, {}, {});
    
    return ComputePipelineBuilder(*vulkanContext)
        .SetShaderModule(shader)
        .Build();
}

void PrimitiveCullStage::BuildDepthPyramidDescriptors()
{
    Assert(depthPyramidReductionDescriptors.empty() && depthPyramidDescriptor == VK_NULL_HANDLE);
    
    const bool singleSample = RenderOptions::Get().GetMsaaSampleCount() == 1;
    const RenderTarget& depthTarget = singleSample ? renderContext->depthTarget : renderContext->depthResolveTarget;
    
    for (uint32_t targetMip = 0; targetMip < depthPyramidRenderTarget.image.GetDescription().mipLevelsCount; ++targetMip)
    {
        const ImageView& sourceView = targetMip == 0 ? (depthTarget.views[0]) : depthPyramidRenderTarget.views[targetMip - 1];
        
        depthPyramidReductionDescriptors.push_back(vulkanContext->GetDescriptorSetsManager()
            .GetReflectiveDescriptorSetBuilder(depthPyramidPipeline, DescriptorScope::eGlobal)
            .Bind("srcDepth", sourceView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, depthPyramidSampler)
            .Bind("dstDepth", depthPyramidRenderTarget, targetMip, VK_IMAGE_LAYOUT_GENERAL)
            .Build()[0]);
    }
    
    depthPyramidDescriptor = vulkanContext->GetDescriptorSetsManager()
        .GetReflectiveDescriptorSetBuilder(secondPassPipeline, DescriptorScope::eGlobal)
        .Bind("depthPyramid", depthPyramidRenderTarget.textureView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, depthPyramidSampler)
        .Build()[0];
}

Pipeline PrimitiveCullStage::BuildSecondPassPipeline() const
{
    std::vector runtimeDefines = { gpu::defines::meshPipeline, gpu::defines::visualizeLods, gpu::defines::drawIndirectCount };
    std::vector<ShaderDefine> defines = { { "FIRST_PASS", 0 } };
    
    ShaderModule shader = GetShader(PrimitiveCullStageDetails::cullShaderPath, VK_SHADER_STAGE_COMPUTE_BIT, runtimeDefines, defines);
    
    return ComputePipelineBuilder(*vulkanContext)
        .SetShaderModule(shader)
        .SetSpecializationConstants({ { "FIRST_PASS", VK_FALSE } })
        .Build();
}

void PrimitiveCullStage::BuildSecondPassDescriptors()
{
    Assert(secondPassDescriptors.empty());
    
    const bool meshPipeline = RenderOptions::Get().GetGraphicsPipelineType() == GraphicsPipelineType::eMesh;
    
    ReflectiveDescriptorSetBuilder builder = vulkanContext->GetDescriptorSetsManager()
        .GetReflectiveDescriptorSetBuilder(firstPassPipeline, DescriptorScope::eSceneRenderer)
        .Bind("Primitives", renderContext->primitiveBuffer)
        .Bind("Draws", renderContext->drawBuffer)
        .Bind("DrawsVisibility", renderContext->drawsVisibilityBuffer)
        .Bind("CommandCount", renderContext->commandCountBuffer)
        .Bind(meshPipeline ? "TaskCommands" : "IndirectCommands", renderContext->commandBuffer)
        .Bind("Primitives", renderContext->primitiveBuffer);
    
    if (RenderOptions::Get().GetVisualizeLods())
    {
        builder.Bind("DrawsDebugData", renderContext->drawsDebugDataBuffer);
    }
    
    secondPassDescriptors = builder.Build();
}
