#include "Engine/Render/RenderStages/PrimitiveCullStage.hpp"

#include "Shaders/Common.h"
#include "Engine/Render/RenderOptions.hpp"
#include "Engine/Render/Vulkan/Pipelines/ComputePipelineBuilder.hpp"
#include "Engine/Render/Vulkan/Synchronization/SynchronizationUtils.hpp"

namespace PrimitiveCullStageDetails
{
    static constexpr std::string_view shaderPath = "~/Shaders/Culling/PrimitiveCull.comp";
}

PrimitiveCullStage::PrimitiveCullStage(const VulkanContext& aVulkanContext, RenderContext& aRenderContext)
    : RenderStage{ aVulkanContext, aRenderContext }
{
    using namespace PrimitiveCullStageDetails;
    
    ShaderModule shaderModule = vulkanContext->GetShaderManager().
        CreateShaderModule(FilePath(shaderPath), VK_SHADER_STAGE_COMPUTE_BIT, renderContext->globalDefines);
    Assert(shaderModule.IsValid());
    
    pipeline = CreatePipeline(std::move(shaderModule));
}

PrimitiveCullStage::~PrimitiveCullStage() = default;

void PrimitiveCullStage::Prepare(const Scene& scene)
{
    CreateDescriptors();
}

void PrimitiveCullStage::Execute(const Frame& frame)
{
    using namespace SynchronizationUtils;

    const VkCommandBuffer cmd = frame.commandBuffer;

    constexpr PipelineBarrier clearCommandCountBarrier = {
        .srcStage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
        .srcAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
        .dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT };

    SetMemoryBarrier(cmd, clearCommandCountBarrier);

    vkCmdFillBuffer(cmd, renderContext->commandCountBuffer, 0, sizeof(uint32_t), 0);

    // TODO: I've seen that driver actually doesn't care about buffer barriers but let's have them, it's nicer
    // TODO: Do you set this as a single barrier or better to have 2 separate ones?
    constexpr PipelineBarrier previousFrameAndClearBarrier = {
        .srcStage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT };

    SetMemoryBarrier(cmd, previousFrameAndClearBarrier);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

    vkCmdPushConstants(cmd, pipeline.GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, 
        static_cast<uint32_t>(sizeof(gpu::PushConstants)), &renderContext->globals);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.GetLayout(), 0,
        static_cast<uint32_t>(descriptors.size()), descriptors.data(), 0, nullptr);

    const auto groupCountX = static_cast<uint32_t>(std::ceil(static_cast<float>(renderContext->globals.drawCount) /
        static_cast<float>(gpu::primitiveCullWgSize)));

    vkCmdDispatch(cmd, groupCountX, 1, 1);

    constexpr PipelineBarrier afterCullBarrier = {
        .srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT,
        .dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_SHADER_READ_BIT };

    SetMemoryBarrier(cmd, afterCullBarrier);
}

bool PrimitiveCullStage::TryReloadShaders()
{
    using namespace PrimitiveCullStageDetails;
    
    reloadedShader = std::make_unique<ShaderModule>(vulkanContext->GetShaderManager().
        CreateShaderModule(FilePath(shaderPath), VK_SHADER_STAGE_COMPUTE_BIT, renderContext->globalDefines));
    
    return reloadedShader->IsValid();
}

void PrimitiveCullStage::ApplyReloadedShaders()
{
    descriptors.clear();
    
    pipeline = CreatePipeline(std::move(*reloadedShader));
    CreateDescriptors();
}

void PrimitiveCullStage::OnSceneClose()
{
    descriptors.clear();
}

Pipeline PrimitiveCullStage::CreatePipeline(ShaderModule&& shaderModule) const
{
    return ComputePipelineBuilder(*vulkanContext)
        .AddPushConstantRange({ VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(gpu::PushConstants) })
        .SetShaderModule(std::move(shaderModule))
        .Build();
}

void PrimitiveCullStage::CreateDescriptors()
{
    Assert(descriptors.empty());
    
    const bool meshPipeline = renderContext->graphicsPipelineType == GraphicsPipelineType::eMesh;
    
    ReflectiveDescriptorSetBuilder builder = vulkanContext->GetDescriptorSetsManager()
        .GetReflectiveDescriptorSetBuilder(pipeline, DescriptorScope::eSceneRenderer)
        .Bind("Primitives", renderContext->primitiveBuffer)
        .Bind("Draws", renderContext->drawBuffer)
        .Bind("CommandCount", renderContext->commandCountBuffer)
        .Bind(meshPipeline ? "TaskCommands" : "IndirectCommands", renderContext->commandBuffer)
        .Bind("Primitives", renderContext->primitiveBuffer);
    
    if (renderContext->visualizeLods)
    {
        builder.Bind("DrawsDebugData", renderContext->drawDebugDataBuffer);
    }
    
    descriptors = builder.Build();
}
