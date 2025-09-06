#include "Engine/Render/RenderStages/DebugStage.hpp"

#include "Engine/Render/RenderOptions.hpp"
#include "Engine/Render/Utils/MeshUtils.hpp"
#include "Engine/Render/Vulkan/VulkanUtils.hpp"
#include "Engine/Render/Vulkan/Buffer/BufferUtils.hpp"
#include "Engine/Render/Vulkan/Pipelines/PipelineUtils.hpp"
#include "Engine/Render/Vulkan/Pipelines/GraphicsPipelineBuilder.hpp"

namespace DebugStageDetails
{
    static constexpr std::string_view boundingSphereVertexPath = "~/Shaders/Debug/BoundingSphere.vert";
    static constexpr std::string_view boundingSphereFragmentPath = "~/Shaders/Debug/BoundingSphere.frag";

    static std::tuple<Buffer, Buffer, uint32_t> CreateUnitSphereBuffers(const VulkanContext& vulkanContext)
    {
        const auto [vertices, indices] = MeshUtils::GenerateSphereMesh(100, 100);
        const auto sphereIndexCount = static_cast<uint32_t>(indices.size());
        
        const std::span verticesSpan = std::as_bytes(std::span(vertices));

        const BufferDescription vertexBufferDescription = {
            .size = verticesSpan.size_bytes(),
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

        auto vertexBuffer = Buffer(vertexBufferDescription, true, verticesSpan, vulkanContext);
        
        const std::span indicesSpan = std::as_bytes(std::span(indices));

        const BufferDescription indexBufferDescription = {
            .size = indicesSpan.size_bytes(),
            .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

        auto indexBuffer = Buffer(indexBufferDescription, true, indicesSpan, vulkanContext);
        
        return { std::move(vertexBuffer), std::move(indexBuffer), sphereIndexCount };
    }
}

DebugStage::DebugStage(const VulkanContext& aVulkanContext, RenderContext& aRenderContext)
    : RenderStage{ aVulkanContext, aRenderContext }
{
    using namespace BufferUtils;
    using namespace DebugStageDetails;
    
    AddShaderInfo({ boundingSphereVertexPath, VK_SHADER_STAGE_VERTEX_BIT, {} });
    AddShaderInfo({ boundingSphereFragmentPath, VK_SHADER_STAGE_FRAGMENT_BIT, {} });
    
    CompileShaders();
    
    boundingSpherePipeline = CreateBoundingSpherePipeline();
    
    std::tie(unitSphereVertexBuffer, unitSphereIndexBuffer, unitSphereIndexCount) = CreateUnitSphereBuffers(*vulkanContext);

    vulkanContext->GetDevice().ExecuteOneTimeCommandBuffer([&](const VkCommandBuffer cmd) {
        CopyBufferToBuffer(cmd, unitSphereVertexBuffer.GetStagingBuffer(), unitSphereVertexBuffer);
        CopyBufferToBuffer(cmd, unitSphereIndexBuffer.GetStagingBuffer(), unitSphereIndexBuffer);
        
        SynchronizationUtils::SetMemoryBarrier(cmd, Barriers::transferWriteToComputeRead);
    });

    unitSphereVertexBuffer.DestroyStagingBuffer();
    unitSphereIndexBuffer.DestroyStagingBuffer();
}

DebugStage::~DebugStage()
{}

void DebugStage::Prepare(const Scene& scene)
{
    CreateBoundingSphereDescriptors();
}

void DebugStage::Execute(const Frame& frame)
{
    using namespace PipelineUtils;
    
    if (!RenderOptions::Get().GetVisualizeBoundingSpheres())
    {
        return;
    }
    
    const VkCommandBuffer commandBuffer = frame.commandBuffer;
    
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, boundingSpherePipeline);

    PushConstants(commandBuffer, boundingSpherePipeline, "globals", renderContext->globals);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, boundingSpherePipeline.GetLayout(), 0,
        static_cast<uint32_t>(boundingSphereDescriptors.size()), boundingSphereDescriptors.data(), 0, nullptr);
    
    const VkBuffer vertexBuffers[] = { unitSphereVertexBuffer };
    const VkDeviceSize offsets[] = { 0 };
    
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, unitSphereIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
    
    for (uint32_t i = 0; i < renderContext->globals.drawCount; ++i)
    {
        vkCmdDrawIndexed(commandBuffer, unitSphereIndexCount, 1, 0, 0, i);
    }
}

void DebugStage::RecreatePipelinesAndDescriptors()
{
    boundingSphereDescriptors.clear();
    boundingSpherePipeline = CreateBoundingSpherePipeline();
    CreateBoundingSphereDescriptors();
}

void DebugStage::OnSceneClose()
{
    boundingSphereDescriptors.clear();
}

Pipeline DebugStage::CreateBoundingSpherePipeline()
{
    using namespace DebugStageDetails;

    const std::vector<VkVertexInputBindingDescription> bindings = { { 0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX } };
    const std::vector<VkVertexInputAttributeDescription> attributes = { { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 } };
    
    return GraphicsPipelineBuilder(*vulkanContext)
        .SetShaderModules({ GetShader(boundingSphereVertexPath), GetShader(boundingSphereFragmentPath) })
        .SetVertexData(bindings, attributes)
        .SetInputTopology(InputTopology::eTriangleList)
        .SetPolygonMode(PolygonMode::eFill)
        .SetCullMode(CullMode::eNone)
        .SetMultisampling(RenderOptions::Get().GetMsaaSampleCount())
        .SetDepthState(true, false, VK_COMPARE_OP_GREATER_OR_EQUAL)
        .SetRenderPass(renderContext->renderPass)
        .EnableBlending()
        .Build();
}

void DebugStage::CreateBoundingSphereDescriptors()
{
    Assert(boundingSphereDescriptors.empty());
    
    boundingSphereDescriptors = vulkanContext->GetDescriptorSetsManager()
        .GetReflectiveDescriptorSetBuilder(boundingSpherePipeline, DescriptorScope::eSceneRenderer)
        .Bind("Draws", renderContext->drawBuffer)
        .Bind("Primitives", renderContext->primitiveBuffer)
        .Build();
}
