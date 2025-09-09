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

    static constexpr std::string_view lineVertexPath = "~/Shaders/Debug/Line.vert";
    static constexpr std::string_view lineFragmentPath = "~/Shaders/Debug/Line.frag";

    static constexpr std::array bindings = { VkVertexInputBindingDescription(0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX) };
    static constexpr std::array attributes = { VkVertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0) };

    static constexpr std::array<uint32_t, 24> frustumLineListIndices = {
        0,1, 1,2, 2,3, 3,0,    // Near
        4,5, 5,6, 6,7, 7,4,    // Far
        0,4, 1,5, 2,6, 3,7, }; // Connections

    static std::tuple<Buffer, Buffer, uint32_t> CreateUnitSphereBuffers(const VulkanContext& vulkanContext)
    {
        const auto [vertices, indices] = MeshUtils::GenerateSphereMesh(100, 100);
        const auto sphereIndexCount = static_cast<uint32_t>(indices.size());
        
        const auto verticesSpan = std::span(vertices);

        const BufferDescription vertexBufferDescription = {
            .size = verticesSpan.size_bytes(),
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

        auto vertexBuffer = Buffer(vertexBufferDescription, true, verticesSpan, vulkanContext);
        
        const auto indicesSpan = std::span(indices);

        const BufferDescription indexBufferDescription = {
            .size = indicesSpan.size_bytes(),
            .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

        auto indexBuffer = Buffer(indexBufferDescription, true, indicesSpan, vulkanContext);
        
        return { std::move(vertexBuffer), std::move(indexBuffer), sphereIndexCount };
    }

    static std::tuple<Buffer, Buffer> CreateFrustumBuffers(const VulkanContext& vulkanContext)
    {
        const BufferDescription vertexBufferDescription = {
            .size = 8 * sizeof(glm::vec3),
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

        auto frustumVertexBuffer = Buffer(vertexBufferDescription, false, vulkanContext);
        
        constexpr auto indicesSpan = std::span(frustumLineListIndices);

        const BufferDescription indexBufferDescription = {
            .size = indicesSpan.size_bytes(),
            .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
        
        auto frustumIndexBuffer = Buffer(indexBufferDescription, true, indicesSpan, vulkanContext);
        
        return { std::move(frustumVertexBuffer), std::move(frustumIndexBuffer) };
    }
}

DebugStage::DebugStage(const VulkanContext& aVulkanContext, RenderContext& aRenderContext)
    : RenderStage{ aVulkanContext, aRenderContext }
{
    using namespace BufferUtils;
    using namespace DebugStageDetails;
    
    AddShaderInfo({ boundingSphereVertexPath, VK_SHADER_STAGE_VERTEX_BIT, {} });
    AddShaderInfo({ boundingSphereFragmentPath, VK_SHADER_STAGE_FRAGMENT_BIT, {} });
    
    AddShaderInfo({ lineVertexPath, VK_SHADER_STAGE_VERTEX_BIT, {} });
    AddShaderInfo({ lineFragmentPath, VK_SHADER_STAGE_FRAGMENT_BIT, {} });
    
    CompileShaders();
    
    boundingSpherePipeline = CreateBoundingSpherePipeline();
    linePipeline = CreateLinePipeline();
    
    std::tie(unitSphereVertexBuffer, unitSphereIndexBuffer, unitSphereIndexCount) = CreateUnitSphereBuffers(*vulkanContext);
    std::tie(frustumVertexBuffer, frustumIndexBuffer) = CreateFrustumBuffers(*vulkanContext);
    
    UploadFromStagingBuffers(*vulkanContext, Barriers::transferWriteToVertexInputRead, /* destroyStagingBuffers */ true,
        unitSphereVertexBuffer,
        unitSphereIndexBuffer,
        frustumIndexBuffer);
}

DebugStage::~DebugStage()
{}

void DebugStage::Prepare(const Scene& scene)
{
    CreateBoundingSphereDescriptors();
}

void DebugStage::Execute(const Frame& frame)
{
    ExecuteBoundingSpheres(frame);
    ExecuteFrozenFrustum(frame);
}

void DebugStage::RecreatePipelinesAndDescriptors()
{
    boundingSphereDescriptors.clear();
    boundingSpherePipeline = CreateBoundingSpherePipeline();
    CreateBoundingSphereDescriptors();
    
    linePipeline = CreateLinePipeline();
}

void DebugStage::OnSceneClose()
{
    boundingSphereDescriptors.clear();
}

void DebugStage::ExecuteBoundingSpheres(const Frame& frame)
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

void DebugStage::ExecuteFrozenFrustum(const Frame& frame)
{
    using namespace PipelineUtils;
    
    if (!RenderOptions::Get().GetFreezeCamera())
    {
        updatedFrustumVertices = false;
        return;
    }
    
    if (!updatedFrustumVertices)
    {
        frustumVertexBuffer.Fill(std::span(renderContext->debugData.frustumCornersWorld));
        updatedFrustumVertices = true;
    }
    
    const VkCommandBuffer commandBuffer = frame.commandBuffer;
    
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, linePipeline);
    PushConstants(commandBuffer, linePipeline, "globals", renderContext->globals);
    
    const VkBuffer vertexBuffers[] = { frustumVertexBuffer };
    const VkDeviceSize offsets[] = { 0 };
    
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, frustumIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
    
    vkCmdDrawIndexed(commandBuffer, DebugStageDetails::frustumLineListIndices.size(), 1, 0, 0, 0);
}

Pipeline DebugStage::CreateBoundingSpherePipeline()
{
    using namespace DebugStageDetails;
    
    return GraphicsPipelineBuilder(*vulkanContext)
        .SetShaderModules({ GetShader(boundingSphereVertexPath), GetShader(boundingSphereFragmentPath) })
        .SetVertexData({ std::from_range, bindings }, { std::from_range, attributes })
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

Pipeline DebugStage::CreateLinePipeline()
{
    using namespace DebugStageDetails;
    
    return GraphicsPipelineBuilder(*vulkanContext)
        .SetShaderModules({ GetShader(lineVertexPath), GetShader(lineFragmentPath) })
        .SetVertexData({ std::from_range, bindings }, { std::from_range, attributes })
        .SetInputTopology(InputTopology::eLineList)
        .SetMultisampling(RenderOptions::Get().GetMsaaSampleCount())
        .SetDepthState(true, false, VK_COMPARE_OP_GREATER_OR_EQUAL)
        .SetRenderPass(renderContext->renderPass)
        .Build();
}
