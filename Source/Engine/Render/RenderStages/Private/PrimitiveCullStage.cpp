#include "Engine/Render/RenderStages/PrimitiveCullStage.hpp"

#include "Shaders/Common.h"
#include "Engine/Render/Vulkan/Buffer/BufferUtils.hpp"
#include "Engine/Render/Vulkan/Pipelines/ComputePipelineBuilder.hpp"
#include "Engine/Render/Vulkan/Synchronization/SynchronizationUtils.hpp"

namespace PrimitiveCullStageDetails
{
    static constexpr std::string_view shaderPath = "~/Shaders/Culling/PrimitiveCull.comp";

    void CreateIndirectBuffers(RenderContext& renderContext, const VulkanContext& vulkanContext)
    {
        using namespace BufferUtils;

        // TODO: Reason about the size carefully, handle out of bounds in shader
        constexpr size_t largeEnoughIndirectBuffer = 1'000'000 * sizeof(gpu::IndirectCommand);

        constexpr BufferDescription commandCountBufferDescription = {
            .size = sizeof(uint32_t),
            .usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

        renderContext.commandCountBuffer = Buffer(commandCountBufferDescription, false, vulkanContext);

        constexpr BufferDescription indirectBufferDescription = {
            .size = largeEnoughIndirectBuffer,
            .usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

        renderContext.indirectBuffer = Buffer(indirectBufferDescription, false, vulkanContext);
    }

    static std::tuple<VkDescriptorSet, DescriptorSetLayout> CreateDescriptors(const RenderContext& renderContext, 
        const VulkanContext& vulkanContext)
    {
        return vulkanContext.GetDescriptorSetsManager().GetDescriptorSetBuilder(DescriptorScope::eSceneRenderer)
            .Bind(0, renderContext.primitiveBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .Bind(1, renderContext.drawBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .Bind(2, renderContext.commandCountBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .Bind(3, renderContext.indirectBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .Build();
    }
}

PrimitiveCullStage::PrimitiveCullStage(const VulkanContext& aVulkanContext, RenderContext& aRenderContext)
    : RenderStage{ aVulkanContext, aRenderContext }
{
    using namespace PrimitiveCullStageDetails;

    CreateIndirectBuffers(*renderContext, *vulkanContext);
}

PrimitiveCullStage::~PrimitiveCullStage() = default;

void PrimitiveCullStage::Prepare(const Scene& scene)
{
    using namespace PrimitiveCullStageDetails;

    std::tie(descriptorSet, descriptorSetLayout) = CreateDescriptors(*renderContext, *vulkanContext);

    if (!pipeline.IsValid())
    {
        pipeline = CreatePipeline();
        Assert(pipeline.IsValid());
    }
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
    constexpr PipelineBarrier previousFrameAndClearBarrier = {
        .srcStage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT };

    SetMemoryBarrier(cmd, previousFrameAndClearBarrier);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

    vkCmdPushConstants(cmd, pipeline.GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, 
        static_cast<uint32_t>(sizeof(gpu::PushConstants)), &renderContext->globals);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.GetLayout(), 0, 1, &descriptorSet, 0, nullptr);

    const auto groupCountX = static_cast<uint32_t>(std::ceil(static_cast<float>(renderContext->globals.drawCount) /
        static_cast<float>(gpu::primitiveCullWgSize)));

    vkCmdDispatch(cmd, groupCountX, 1, 1);
}

void PrimitiveCullStage::TryReloadShaders()
{
    if (Pipeline newPipeline = CreatePipeline(); newPipeline.IsValid())
    {
        pipeline = std::move(newPipeline);
    }
}

Pipeline PrimitiveCullStage::CreatePipeline() const
{
    using namespace PrimitiveCullStageDetails;

    ShaderModule shaderModule = vulkanContext->GetShaderManager().CreateShaderModule(FilePath(shaderPath), 
        ShaderType::eCompute);

    if (!shaderModule.IsValid())
    {
        return {};
    }

    std::vector<VkDescriptorSetLayout> layouts = { descriptorSetLayout };

    return ComputePipelineBuilder(*vulkanContext)
        .SetDescriptorSetLayouts(std::move(layouts)) // TODO: Why telling stage here?? We need reflection...
        .AddPushConstantRange({ VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(gpu::PushConstants) })
        .SetShaderModule(std::move(shaderModule))
        .Build();
}