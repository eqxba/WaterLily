#include "Engine/Render/RenderStages/ForwardStage.hpp"

#include "Engine/Scene/SceneHelpers.hpp"
#include "Engine/Render/RenderOptions.hpp"
#include "Engine/Render/Vulkan/VulkanUtils.hpp"
#include "Engine/Render/Vulkan/Pipelines/PipelineUtils.hpp"
#include "Engine/Render/Vulkan/Pipelines/GraphicsPipelineBuilder.hpp"

namespace ForwardStageDetails
{
    static constexpr std::string_view vertexShaderPath = "~/Shaders/Default.vert";
    static constexpr std::string_view taskShaderPath = "~/Shaders/Meshlet.task";
    static constexpr std::string_view meshShaderPath = "~/Shaders/Meshlet.mesh";
    static constexpr std::string_view fragmentShaderPath = "~/Shaders/Default.frag";
}

ForwardStage::ForwardStage(const VulkanContext& aVulkanContext, RenderContext& aRenderContext)
    : RenderStage{ aVulkanContext, aRenderContext }
{
    if (vulkanContext->GetDevice().GetProperties().meshShadersSupported)
    {
        AddPipeline(graphicsPipelines[GraphicsPipelineType::eMesh], [&]() { return BuildMeshPipeline(); });
    }
    
    AddPipeline(graphicsPipelines[GraphicsPipelineType::eVertex], [&]() { return BuildVertexPipeline(); });
}

ForwardStage::~ForwardStage()
{}

void ForwardStage::OnSceneOpen(const Scene& scene)
{
    BuildDescriptors();
}

void ForwardStage::OnSceneClose()
{
    descriptors.clear();
}

void ForwardStage::Execute(const Frame& frame)
{
    using namespace VulkanUtils;
    using namespace PipelineUtils;
    using namespace ForwardStageDetails;
    
    const VkCommandBuffer commandBuffer = frame.commandBuffer;
    
    const GraphicsPipelineType pipelineType = RenderOptions::Get().GetGraphicsPipelineType();
    const Pipeline& graphicsPipeline = graphicsPipelines[pipelineType];
    const std::vector<VkDescriptorSet>& descriptorSets = descriptors[pipelineType];
    
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    PushConstants(commandBuffer, graphicsPipeline, "globals", renderContext->globals);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetLayout(), 0,
        static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);

    if (pipelineType == GraphicsPipelineType::eMesh)
    {
        ExecuteMesh(frame);
    }
    else
    {
        ExecuteVertex(frame);
    }
}

void ForwardStage::RebuildDescriptors()
{
    descriptors.clear();
    
    BuildDescriptors();
}

Pipeline ForwardStage::BuildMeshPipeline()
{
    using namespace ForwardStageDetails;
    
    std::vector<ShaderModule> shaders;
    
    std::vector runtimeDefines = { gpu::defines::visualizeLods };
    
    shaders.push_back(GetShader(taskShaderPath, VK_SHADER_STAGE_TASK_BIT_EXT, {}, {}));
    shaders.push_back(GetShader(meshShaderPath, VK_SHADER_STAGE_MESH_BIT_EXT, {}, {}));
    shaders.push_back(GetShader(fragmentShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT, runtimeDefines, {}));

    return GraphicsPipelineBuilder(*vulkanContext)
        .SetShaderModules(shaders)
        .SetPolygonMode(PolygonMode::eFill)
        .SetMultisampling(RenderOptions::Get().GetMsaaSampleCount())
        .SetDepthState(true, true, VK_COMPARE_OP_GREATER_OR_EQUAL)
        .SetRenderPass(renderContext->firstRenderPass)
        .Build();
}

Pipeline ForwardStage::BuildVertexPipeline()
{
    using namespace ForwardStageDetails;
    
    std::vector runtimeDefines = { gpu::defines::visualizeLods };
    
    std::vector<ShaderModule> shaders;
    
    shaders.push_back(GetShader(vertexShaderPath, VK_SHADER_STAGE_VERTEX_BIT, runtimeDefines, {}));
    shaders.push_back(GetShader(fragmentShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT, runtimeDefines, {}));
    
    return GraphicsPipelineBuilder(*vulkanContext)
        .SetShaderModules(shaders)
        .SetVertexData(SceneHelpers::GetVertexBindings(), SceneHelpers::GetVertexAttributes())
        .SetInputTopology(InputTopology::eTriangleList)
        .SetPolygonMode(PolygonMode::eFill)
        .SetCullMode(CullMode::eBack, false)
        .SetMultisampling(RenderOptions::Get().GetMsaaSampleCount())
        .SetDepthState(true, true, VK_COMPARE_OP_GREATER_OR_EQUAL)
        .SetRenderPass(renderContext->firstRenderPass)
        .Build();
}

void ForwardStage::BuildDescriptors()
{
    Assert(descriptors.empty());
    
    if (graphicsPipelines.contains(GraphicsPipelineType::eMesh))
    {
        descriptors[GraphicsPipelineType::eMesh] = vulkanContext->GetDescriptorSetsManager()
            .GetReflectiveDescriptorSetBuilder(graphicsPipelines[GraphicsPipelineType::eMesh], DescriptorScope::eSceneRenderer)
            .Bind("Vertices", renderContext->vertexBuffer)
            .Bind("MeshletData32", renderContext->meshletDataBuffer)
            .Bind("Meshlets", renderContext->meshletBuffer)
            .Bind("Draws", renderContext->drawBuffer)
            .Bind("TaskCommands", renderContext->commandBuffer)
            .Build();
    }
    
    ReflectiveDescriptorSetBuilder builder = vulkanContext->GetDescriptorSetsManager()
        .GetReflectiveDescriptorSetBuilder(graphicsPipelines[GraphicsPipelineType::eVertex], DescriptorScope::eSceneRenderer)
        .Bind("Draws", renderContext->drawBuffer);
    
    if (RenderOptions::Get().GetVisualizeLods())
    {
        builder.Bind("DrawsDebugData", renderContext->drawsDebugDataBuffer);
    }
    
    descriptors[GraphicsPipelineType::eVertex] = builder.Build();
}

void ForwardStage::ExecuteMesh(const Frame& frame) const
{
    vkCmdDrawMeshTasksIndirectEXT(frame.commandBuffer, renderContext->commandCountBuffer, 0, 1, 0);
}

void ForwardStage::ExecuteVertex(const Frame& frame) const
{
    const VkCommandBuffer commandBuffer = frame.commandBuffer;
    
    const VkBuffer vertexBuffers[] = { renderContext->vertexBuffer };
    const VkDeviceSize offsets[] = { 0 };
    
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, renderContext->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    
    if (vulkanContext->GetDevice().GetProperties().drawIndirectCountSupported)
    {
        vkCmdDrawIndexedIndirectCount(commandBuffer, renderContext->commandBuffer, 0, renderContext->commandCountBuffer, 
            0, renderContext->globals.drawCount, sizeof(gpu::VkDrawIndexedIndirectCommand));
    }
    else
    {
        vkCmdDrawIndexedIndirect(commandBuffer, renderContext->commandBuffer, 0, renderContext->globals.drawCount, 
            sizeof(gpu::VkDrawIndexedIndirectCommand));
    }
}
