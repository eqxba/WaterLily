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

    static std::vector<ShaderModule> CreateShaderModules(const GraphicsPipelineType type,
        const RenderContext& renderContext, const VulkanContext& vulkanContext)
    {
        const ShaderManager& shaderManager = vulkanContext.GetShaderManager();

        std::vector<ShaderModule> shaderModules;

        if (type == GraphicsPipelineType::eMesh)
        {
            shaderModules.emplace_back(shaderManager.CreateShaderModule(FilePath(taskShaderPath), VK_SHADER_STAGE_TASK_BIT_EXT, renderContext.globalDefines));
            shaderModules.emplace_back(shaderManager.CreateShaderModule(FilePath(meshShaderPath), VK_SHADER_STAGE_MESH_BIT_EXT, renderContext.globalDefines));
        }
        else
        {
            shaderModules.emplace_back(shaderManager.CreateShaderModule(FilePath(vertexShaderPath), VK_SHADER_STAGE_VERTEX_BIT, renderContext.globalDefines));
        }

        shaderModules.emplace_back(shaderManager.CreateShaderModule(FilePath(fragmentShaderPath), VK_SHADER_STAGE_FRAGMENT_BIT, renderContext.globalDefines));

        return shaderModules;
    }
}

ForwardStage::ForwardStage(const VulkanContext& aVulkanContext, RenderContext& aRenderContext)
    : RenderStage{ aVulkanContext, aRenderContext }
{
    using namespace ForwardStageDetails;
    
    if (vulkanContext->GetDevice().GetProperties().meshShadersSupported)
    {
        shaders[GraphicsPipelineType::eMesh] = CreateShaderModules(GraphicsPipelineType::eMesh, *renderContext, *vulkanContext);
        graphicsPipelines[GraphicsPipelineType::eMesh] = CreateMeshPipeline(shaders[GraphicsPipelineType::eMesh]);
    }
    
    shaders[GraphicsPipelineType::eVertex] = CreateShaderModules(GraphicsPipelineType::eVertex, *renderContext, *vulkanContext);
    graphicsPipelines[GraphicsPipelineType::eVertex] = CreateVertexPipeline(shaders[GraphicsPipelineType::eVertex]);
}

ForwardStage::~ForwardStage()
{}

void ForwardStage::Prepare(const Scene& scene)
{
    CreateDescriptors();
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

bool ForwardStage::TryReloadShaders()
{
    using namespace ForwardStageDetails;
    
    bool success = true;
    
    for (const auto& [type, pipeline] : graphicsPipelines)
    {
        reloadedShaders[type] = CreateShaderModules(type, *renderContext, *vulkanContext);
        success = success && std::ranges::all_of(reloadedShaders[type], &ShaderModule::IsValid);
    }
    
    return success;
}

void ForwardStage::RecreatePipelinesAndDescriptors()
{
    descriptors.clear();
    
    if (!reloadedShaders.empty())
    {
        shaders = std::move(reloadedShaders);
    }
    
    for (auto& [type, pipeline] : graphicsPipelines)
    {
        pipeline = type == GraphicsPipelineType::eMesh ? CreateMeshPipeline(shaders[type]) : CreateVertexPipeline(shaders[type]);
    }
    
    CreateDescriptors();
}

void ForwardStage::OnSceneClose()
{
    descriptors.clear();
}

Pipeline ForwardStage::CreateMeshPipeline(const std::vector<ShaderModule>& shaderModules)
{
    using namespace ForwardStageDetails;

    return GraphicsPipelineBuilder(*vulkanContext)
        .SetShaderModules(shaderModules)
        .SetPolygonMode(PolygonMode::eFill)
        .SetMultisampling(RenderOptions::Get().GetMsaaSampleCount())
        .SetDepthState(true, true, VK_COMPARE_OP_GREATER_OR_EQUAL)
        .SetRenderPass(renderContext->renderPass)
        .Build();
}

Pipeline ForwardStage::CreateVertexPipeline(const std::vector<ShaderModule>& shaderModules)
{
    using namespace ForwardStageDetails;
    
    return GraphicsPipelineBuilder(*vulkanContext)
        .SetShaderModules(shaderModules)
        .SetVertexData(SceneHelpers::GetVertexBindings(), SceneHelpers::GetVertexAttributes())
        .SetInputTopology(InputTopology::eTriangleList)
        .SetPolygonMode(PolygonMode::eFill)
        .SetCullMode(CullMode::eBack, false)
        .SetMultisampling(RenderOptions::Get().GetMsaaSampleCount())
        .SetDepthState(true, true, VK_COMPARE_OP_GREATER_OR_EQUAL)
        .SetRenderPass(renderContext->renderPass)
        .Build();
}

void ForwardStage::CreateDescriptors()
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
    
    if (renderContext->visualizeLods)
    {
        builder.Bind("DrawsDebugData", renderContext->drawDebugDataBuffer);
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
