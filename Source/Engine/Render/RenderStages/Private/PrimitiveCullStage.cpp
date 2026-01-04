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
}

PrimitiveCullStage::PrimitiveCullStage(const VulkanContext& aVulkanContext, RenderContext& aRenderContext)
    : RenderStage{ aVulkanContext, aRenderContext }
{
    // TODO: Keep only required ones
    AddPipeline(pipeline, [&]() { return BuildPipeline(false); });
    AddPipeline(firstPassPipeline, [&]() { return BuildPipeline(true, true); });
    AddPipeline(depthPyramidPipeline, [&]() { return BuildDepthPyramidPipeline(); });
    AddPipeline(secondPassPipeline, [&]() { return BuildPipeline(true, false); });
}

PrimitiveCullStage::~PrimitiveCullStage() = default;

void PrimitiveCullStage::CreateRenderTargetDependentResources()
{
    if (RenderOptions::Get().GetOcclusionCulling())
    {
        CreateDepthPyramidRenderTargetAndSampler();
        BuildDepthPyramidDescriptors();
    }
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
    if (RenderOptions::Get().GetOcclusionCulling())
    {
        firstPassDescriptors = BuildDescriptors(firstPassPipeline);
        secondPassDescriptors = BuildDescriptors(secondPassPipeline);
    }
    else
    {
        descriptors = BuildDescriptors(pipeline);
    }
}

void PrimitiveCullStage::OnSceneClose()
{
    descriptors.clear();
    firstPassDescriptors.clear();
    secondPassDescriptors.clear();
}

void PrimitiveCullStage::Execute(const Frame& frame)
{
    Assert(!RenderOptions::Get().GetOcclusionCulling()); // Use dedicated Execute methods (ExecuteFirstPass, BuildDepthPyramid, ExecuteSecondPass);
    
    using namespace SynchronizationUtils;
    using namespace PipelineUtils;

    const VkCommandBuffer cmd = frame.commandBuffer;

    StatsUtils::WriteTimestamp(cmd, frame.queryPools.timestamps, GpuTimestamp::eFirstCullingPassBegin);
    
    SetMemoryBarrier(cmd, Barriers::indirectCommandReadToTransferWrite);
    vkCmdFillBuffer(cmd, renderContext->commandCountBuffer, 0, sizeof(uint32_t), 0);
    SetMemoryBarrier(cmd, Barriers::transferWriteToComputeReadWrite);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

    PushConstants(cmd, pipeline, "globals", renderContext->globals);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.GetLayout(), 0, static_cast<uint32_t>(descriptors.size()), descriptors.data(), 0, nullptr);
    
    vkCmdDispatch(cmd, GroupCount(renderContext->globals.drawCount, gpu::primitiveCullWgSize), 1, 1);

    SetMemoryBarrier(cmd, RenderOptions::Get().GetGraphicsPipelineType() == GraphicsPipelineType::eMesh
        ? Barriers::computeWriteToIndirectCommandRead | Barriers::computeWriteToTaskRead : Barriers::computeWriteToIndirectCommandRead);
    
    StatsUtils::WriteTimestamp(cmd, frame.queryPools.timestamps, GpuTimestamp::eFirstCullingPassEnd);
}

void PrimitiveCullStage::RebuildDescriptors()
{
    firstPassDescriptors.clear();
    depthPyramidReductionDescriptors.clear();
    depthPyramidDescriptor = VK_NULL_HANDLE;
    secondPassDescriptors.clear();
    
    if (RenderOptions::Get().GetOcclusionCulling())
    {
        firstPassDescriptors = BuildDescriptors(firstPassPipeline);
        BuildDepthPyramidDescriptors();
        secondPassDescriptors = BuildDescriptors(secondPassPipeline);
    }
    else
    {
        descriptors = BuildDescriptors(pipeline);
    }
}

void PrimitiveCullStage::ExecuteFirstPass(const Frame& frame)
{
    using namespace SynchronizationUtils;
    using namespace PipelineUtils;

    const VkCommandBuffer cmd = frame.commandBuffer;
    
    StatsUtils::WriteTimestamp(cmd, frame.queryPools.timestamps, GpuTimestamp::eFirstCullingPassBegin);

    SetMemoryBarrier(cmd, Barriers::indirectCommandReadToTransferWrite);
    vkCmdFillBuffer(cmd, renderContext->commandCountBuffer, 0, sizeof(uint32_t), 0);
    SetMemoryBarrier(cmd, Barriers::transferWriteToComputeReadWrite);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, firstPassPipeline);

    PushConstants(cmd, firstPassPipeline, "globals", renderContext->globals);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, firstPassPipeline.GetLayout(), 0,
        static_cast<uint32_t>(firstPassDescriptors.size()), firstPassDescriptors.data(), 0, nullptr);
    
    vkCmdDispatch(cmd, GroupCount(renderContext->globals.drawCount, gpu::primitiveCullWgSize), 1, 1);

    SetMemoryBarrier(cmd, RenderOptions::Get().GetGraphicsPipelineType() == GraphicsPipelineType::eMesh
        ? Barriers::computeWriteToIndirectCommandRead | Barriers::computeWriteToTaskRead : Barriers::computeWriteToIndirectCommandRead);
    
    StatsUtils::WriteTimestamp(frame.commandBuffer, frame.queryPools.timestamps, GpuTimestamp::eFirstCullingPassEnd);
}

void PrimitiveCullStage::BuildDepthPyramid(const Frame& frame)
{
    using namespace ImageUtils;
    using namespace PipelineUtils;
    using namespace SynchronizationUtils;
    
    const VkCommandBuffer cmd = frame.commandBuffer;
    const VkExtent3D pyramidExtent = depthPyramidRenderTarget.image.GetDescription().extent;
    
    StatsUtils::WriteTimestamp(frame.commandBuffer, frame.queryPools.timestamps, GpuTimestamp::eDepthPyramidBegin);
    
    // Block our sampling until we have depth RT output finished / depth resolve RT resolved in the 1st pass
    SetMemoryBarrier(cmd, RenderOptions::Get().GetMsaaSampleCount() == 1 ? Barriers::lateDepthStencilWriteToComputeRead : Barriers::resolveToComputeRead);
    
    // Prepare pyramid RT for writing into it, block on pyramid reads from previous frame (2nd pass)
    TransitionLayout(cmd, depthPyramidRenderTarget, LayoutTransitions::shaderReadOnlyOptimalToGeneral, Barriers::computeReadToComputeWrite);
    
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, depthPyramidPipeline);
    
    for (uint32_t targetMip = 0; targetMip < depthPyramidRenderTarget.image.GetDescription().mipLevelsCount; ++targetMip)
    {
        const uint32_t dstWidth = std::max(1u, pyramidExtent.width >> targetMip);
        const uint32_t dstHeight = std::max(1u, pyramidExtent.height >> targetMip);
        
        PushConstants(cmd, depthPyramidPipeline, "dstSize", glm::vec2(dstWidth, dstHeight));
        
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, depthPyramidPipeline.GetLayout(), 0,
            1, &depthPyramidReductionDescriptors[targetMip], 0, nullptr);
        
        vkCmdDispatch(cmd, GroupCount(dstWidth, gpu::depthPyramidWgSize), GroupCount(dstHeight, gpu::depthPyramidWgSize), 1);
        
        TransitionLayout(cmd, depthPyramidRenderTarget, LayoutTransitions::generalToShaderReadOnlyOptimal, Barriers::computeWriteToComputeRead, targetMip);
    }
    
    StatsUtils::WriteTimestamp(frame.commandBuffer, frame.queryPools.timestamps, GpuTimestamp::eDepthPyramidEnd);
}

void PrimitiveCullStage::VisualizeDepth(const Frame& frame)
{
    using namespace ImageUtils;
    
    const RenderTarget& targetRt = vulkanContext->GetSwapchain().GetRenderTargets()[frame.swapchainImageIndex];
    
    TransitionLayout(frame.commandBuffer, depthPyramidRenderTarget, LayoutTransitions::shaderReadOnlyOptimalToSrcOptimal,
        Barriers::computeReadToTransferRead);
    
    TransitionLayout(frame.commandBuffer, targetRt, LayoutTransitions::colorAttachmentOptimalToDstOptimal,
        Barriers::colorReadWriteToTransferWrite);

    BlitImageToImage(frame.commandBuffer, depthPyramidRenderTarget, targetRt, RenderOptions::Get().GetDepthMipToVisualize(),
        0, VK_FILTER_NEAREST);
    
    TransitionLayout(frame.commandBuffer, depthPyramidRenderTarget, LayoutTransitions::srcOptimalToShaderReadOnlyOptimal,
        Barriers::transferReadToComputeRead);
    
    TransitionLayout(frame.commandBuffer, targetRt, LayoutTransitions::dstOptimalToColorAttachmentOptimal,
        Barriers::transferWriteToColorReadWrite);
}

void PrimitiveCullStage::ExecuteSecondPass(const Frame& frame)
{
    using namespace SynchronizationUtils;
    using namespace PipelineUtils;

    const VkCommandBuffer cmd = frame.commandBuffer;
    
    StatsUtils::WriteTimestamp(cmd, frame.queryPools.timestamps, GpuTimestamp::eSecondCullingPassBegin);

    SetMemoryBarrier(cmd, Barriers::indirectCommandReadToTransferWrite);
    vkCmdFillBuffer(cmd, renderContext->commandCountBuffer, 0, sizeof(uint32_t), 0);
    SetMemoryBarrier(cmd, Barriers::transferWriteToComputeReadWrite);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, secondPassPipeline);

    PushConstants(cmd, secondPassPipeline, "globals", renderContext->globals);
    
    std::vector<VkDescriptorSet> passDescriptors = secondPassDescriptors;
    passDescriptors.push_back(depthPyramidDescriptor);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, secondPassPipeline.GetLayout(), 0,
        static_cast<uint32_t>(passDescriptors.size()), passDescriptors.data(), 0, nullptr);

    const auto groupCountX = static_cast<uint32_t>(std::ceil(static_cast<float>(renderContext->globals.drawCount) /
        static_cast<float>(gpu::primitiveCullWgSize)));

    vkCmdDispatch(cmd, groupCountX, 1, 1);

    SetMemoryBarrier(cmd, RenderOptions::Get().GetGraphicsPipelineType() == GraphicsPipelineType::eMesh
        ? Barriers::computeWriteToIndirectCommandRead | Barriers::computeWriteToTaskRead : Barriers::computeWriteToIndirectCommandRead);
    
    StatsUtils::WriteTimestamp(cmd, frame.queryPools.timestamps, GpuTimestamp::eSecondCullingPassEnd);
}

Pipeline PrimitiveCullStage::BuildPipeline(const bool occlusionCulling /* = true */, const bool firstPass /* = true */) const
{
    std::vector runtimeDefines = { gpu::defines::meshPipeline, gpu::defines::visualizeLods, gpu::defines::drawIndirectCount };
    std::vector<ShaderDefine> defines = { { "OCCLUSION_CULLING", occlusionCulling }, { "FIRST_PASS", firstPass } };
    
    ShaderModule shader = GetShader(PrimitiveCullStageDetails::cullShaderPath, VK_SHADER_STAGE_COMPUTE_BIT, runtimeDefines, defines);
    
    return ComputePipelineBuilder(*vulkanContext)
        .SetShaderModule(shader)
        .Build();
}

std::vector<VkDescriptorSet> PrimitiveCullStage::BuildDescriptors(const Pipeline& aPipeline)
{
    const bool meshPipeline = RenderOptions::Get().GetGraphicsPipelineType() == GraphicsPipelineType::eMesh;
    
    ReflectiveDescriptorSetBuilder builder = vulkanContext->GetDescriptorSetsManager()
        .GetReflectiveDescriptorSetBuilder(aPipeline, DescriptorScope::eSceneRenderer)
        .Bind("Primitives", renderContext->primitiveBuffer)
        .Bind("Draws", renderContext->drawBuffer)
        .Bind("CommandCount", renderContext->commandCountBuffer)
        .Bind(meshPipeline ? "TaskCommands" : "IndirectCommands", renderContext->commandBuffer)
        .Bind("Primitives", renderContext->primitiveBuffer);
    
    if (aPipeline.HasBinding("DrawsVisibility"))
    {
        builder.Bind("DrawsVisibility", renderContext->drawsVisibilityBuffer);
    }
    
    if (RenderOptions::Get().GetVisualizeLods())
    {
        builder.Bind("DrawsDebugData", renderContext->drawsDebugDataBuffer);
    }
    
    return builder.Build();
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
        ImageUtils::TransitionLayout(cmd, depthPyramidRenderTarget, LayoutTransitions::undefinedToShaderReadOnlyOptimal, Barriers::noneToComputeWrite);
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
