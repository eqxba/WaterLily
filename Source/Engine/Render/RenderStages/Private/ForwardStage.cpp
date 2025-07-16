#include "Engine/Render/RenderStages/ForwardStage.hpp"

#include "Engine/Scene/SceneHelpers.hpp"
#include "Engine/Render/Vulkan/VulkanUtils.hpp"
#include "Engine/Render/Vulkan/Pipelines/GraphicsPipelineBuilder.hpp"

namespace ForwardStageDetails
{
    static constexpr std::string_view vertexShaderPath = "~/Shaders/Default.vert";
    static constexpr std::string_view taskShaderPath = "~/Shaders/Meshlet.task";
    static constexpr std::string_view meshShaderPath = "~/Shaders/Meshlet.mesh";
    static constexpr std::string_view fragmentShaderPath = "~/Shaders/Default.frag";

    static RenderPass CreateRenderPass(const VulkanContext& vulkanContext)
    {
        AttachmentDescription colorAttachmentDescription = {
            .format = vulkanContext.GetSwapchain().GetSurfaceFormat().format,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .actualLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        AttachmentDescription resolveAttachmentDescription = {
            .format = vulkanContext.GetSwapchain().GetSurfaceFormat().format,
            .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .actualLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        AttachmentDescription depthStencilAttachmentDescription = {
            .format = VulkanConfig::depthImageFormat,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .actualLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
        
        std::vector<PipelineBarrier> previousBarriers = { {
            // Wait for any previous depth output
            .srcStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT }, {
            // Wait for any previous color output
            .srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT } };
        
        std::vector<PipelineBarrier> followingBarriers = { {
            // Make UI renderer wait for our color output
            .srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT } };

        return RenderPassBuilder(vulkanContext)
            .SetBindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS)
            .SetMultisampling(vulkanContext.GetDevice().GetProperties().maxSampleCount)
            .AddColorAndResolveAttachments(colorAttachmentDescription, resolveAttachmentDescription)
            .AddDepthStencilAttachment(depthStencilAttachmentDescription)
            .SetPreviousBarriers(std::move(previousBarriers))
            .SetFollowingBarriers(std::move(followingBarriers))
            .Build();
    }

    static std::vector<VkFramebuffer> CreateFramebuffers(const RenderPass& renderPass,
        const VulkanContext& vulkanContext, const RenderContext& renderContext)
    {
        std::vector<VkFramebuffer> framebuffers;
        
        const Swapchain& swapchain = vulkanContext.GetSwapchain();
        
        const std::vector<RenderTarget>& swapchainTargets = swapchain.GetRenderTargets();
        framebuffers.reserve(swapchainTargets.size());

        std::vector<VkImageView> attachments = { renderContext.colorTarget.view, VK_NULL_HANDLE, renderContext.depthTarget.view };

        std::ranges::transform(swapchainTargets, std::back_inserter(framebuffers), [&](const RenderTarget& target) {
            attachments[1] = target.view;
            return VulkanUtils::CreateFrameBuffer(renderPass, swapchain.GetExtent(), attachments, vulkanContext);
        });
        
        return framebuffers;
    }

    static DescriptorSetLayout GetMeshDescriptorSetLayout(const VulkanContext& vulkanContext)
    {
        // TODO: Parse from SPIR-V reflection
        return vulkanContext.GetDescriptorSetsManager().GetDescriptorSetLayoutBuilder()
            .AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT) // Vertices
            .AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT) // MeshletData
            .AddBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT) // Meshlets
            .AddBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT) // Draws
            .AddBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_TASK_BIT_EXT) // TaskCommands
            .Build();
    }

    static DescriptorSetLayout GetVertexDescriptorSetLayout(const VulkanContext& vulkanContext)
    {
        return vulkanContext.GetDescriptorSetsManager().GetDescriptorSetLayoutBuilder()
            .AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT) // Draws
            .AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT) // DrawsDebugData // TODO: Optional?
            .Build();
    }

    static std::vector<ShaderModule> GetShaderModules(const GraphicsPipelineType type,
        const RenderContext& renderContext, const VulkanContext& vulkanContext)
    {
        const ShaderManager& shaderManager = vulkanContext.GetShaderManager();

        std::vector<ShaderModule> shaderModules;

        if (type == GraphicsPipelineType::eMesh)
        {
            shaderModules.emplace_back(shaderManager.CreateShaderModule(FilePath(taskShaderPath), ShaderType::eTask, renderContext.globalDefines));
            shaderModules.emplace_back(shaderManager.CreateShaderModule(FilePath(meshShaderPath), ShaderType::eMesh, renderContext.globalDefines));
        }
        else
        {
            shaderModules.emplace_back(shaderManager.CreateShaderModule(FilePath(vertexShaderPath), ShaderType::eVertex, renderContext.globalDefines));
        }

        shaderModules.emplace_back(shaderManager.CreateShaderModule(FilePath(fragmentShaderPath), ShaderType::eFragment, renderContext.globalDefines));

        return shaderModules;
    }
}

ForwardStage::ForwardStage(const VulkanContext& aVulkanContext, RenderContext& aRenderContext)
    : RenderStage{ aVulkanContext, aRenderContext }
{
    using namespace ForwardStageDetails;
    
    renderPass = CreateRenderPass(*vulkanContext);
    framebuffers = CreateFramebuffers(renderPass, *vulkanContext, *renderContext);
    
    if (vulkanContext->GetDevice().GetProperties().meshShadersSupported)
    {
        descriptors[GraphicsPipelineType::eMesh] = { VK_NULL_HANDLE, GetMeshDescriptorSetLayout(*vulkanContext) };
        
        graphicsPipelines[GraphicsPipelineType::eMesh] = CreateMeshPipeline();
        Assert(graphicsPipelines[GraphicsPipelineType::eMesh].IsValid());
    }
    
    descriptors[GraphicsPipelineType::eVertex] = { VK_NULL_HANDLE, GetVertexDescriptorSetLayout(*vulkanContext) };
    
    graphicsPipelines[GraphicsPipelineType::eVertex] = CreateVertexPipeline();
    Assert(graphicsPipelines[GraphicsPipelineType::eVertex].IsValid());
}

ForwardStage::~ForwardStage()
{
    VulkanUtils::DestroyFramebuffers(framebuffers, *vulkanContext);
}

void ForwardStage::Prepare(const Scene& scene)
{
    DescriptorSetManager& descriptorSetManager = vulkanContext->GetDescriptorSetsManager();
    
    if (descriptors.contains(GraphicsPipelineType::eMesh))
    {
        auto& pair = descriptors[GraphicsPipelineType::eMesh];
        
        pair = descriptorSetManager.GetDescriptorSetBuilder(pair.second, DescriptorScope::eSceneRenderer)
            .Bind(0, renderContext->vertexBuffer)
            .Bind(1, renderContext->meshletDataBuffer)
            .Bind(2, renderContext->meshletBuffer)
            .Bind(3, renderContext->drawBuffer)
            .Bind(4, renderContext->commandBuffer)
            .Build();
    }
    
    auto& pair = descriptors[GraphicsPipelineType::eVertex];
    
    pair = descriptorSetManager.GetDescriptorSetBuilder(pair.second, DescriptorScope::eSceneRenderer)
        .Bind(0, renderContext->drawBuffer)
        .Bind(1, renderContext->drawDebugDataBuffer)
        .Build();
}

void ForwardStage::Execute(const Frame& frame)
{
    using namespace VulkanUtils;
    
    const GraphicsPipelineType pipelineType = RenderOptions::Get().GetGraphicsPipelineType();
    const Pipeline& graphicsPipeline = graphicsPipelines[pipelineType];
    
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffers[frame.swapchainImageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = vulkanContext->GetSwapchain().GetExtent();

    std::array<VkClearValue, 3> clearValues{};
    clearValues[0].color = { { 0.73f, 0.95f, 1.0f, 1.0f } };
    clearValues[2].depthStencil = { 0.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    
    const VkCommandBuffer commandBuffer = frame.commandBuffer;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    const VkExtent2D extent = vulkanContext->GetSwapchain().GetExtent();
    VkViewport viewport = GetViewport(static_cast<float>(extent.width), static_cast<float>(extent.height));
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    const VkRect2D scissor = GetScissor(extent);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdPushConstants(commandBuffer, graphicsPipeline.GetLayout(),
        pipelineType == GraphicsPipelineType::eMesh ? VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT :
        VK_SHADER_STAGE_VERTEX_BIT, 0, static_cast<uint32_t>(sizeof(gpu::PushConstants)), &renderContext->globals);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetLayout(),
        0, 1, &descriptors[pipelineType].first, 0, nullptr);

    if (pipelineType == GraphicsPipelineType::eMesh)
    {
        ExecuteMesh(frame);
    }
    else
    {
        ExecuteVertex(frame);
    }

    vkCmdEndRenderPass(commandBuffer);
}

void ForwardStage::RecreateFramebuffers()
{
    VulkanUtils::DestroyFramebuffers(framebuffers, *vulkanContext);
    framebuffers = ForwardStageDetails::CreateFramebuffers(renderPass, *vulkanContext, *renderContext);
}

void ForwardStage::TryReloadShaders()
{
    using namespace ForwardStageDetails;
    
    for (auto& [type, pipeline] : graphicsPipelines)
    {
        Pipeline newPipeline = type == GraphicsPipelineType::eMesh ? CreateMeshPipeline() : CreateVertexPipeline();
        
        if (newPipeline.IsValid())
        {
            pipeline = std::move(newPipeline);
        }
    }
}

Pipeline ForwardStage::CreateMeshPipeline()
{
    using namespace ForwardStageDetails;
    
    std::vector<ShaderModule> shaderModules = GetShaderModules(GraphicsPipelineType::eMesh, *renderContext, *vulkanContext);
    
    if (!std::ranges::all_of(shaderModules, &ShaderModule::IsValid))
    {
        return {};
    }

    return GraphicsPipelineBuilder(*vulkanContext)
        .SetDescriptorSetLayouts({ descriptors[GraphicsPipelineType::eMesh].second })
        .AddPushConstantRange({ VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT, 0, sizeof(gpu::PushConstants) })
        .SetShaderModules(std::move(shaderModules))
        .SetPolygonMode(PolygonMode::eFill)
        .SetMultisampling(vulkanContext->GetDevice().GetProperties().maxSampleCount)
        .SetDepthState(true, true, VK_COMPARE_OP_GREATER_OR_EQUAL)
        .SetRenderPass(renderPass)
        .Build();
}

Pipeline ForwardStage::CreateVertexPipeline()
{
    using namespace ForwardStageDetails;
    
    std::vector<ShaderModule> shaderModules = GetShaderModules(GraphicsPipelineType::eVertex, *renderContext, *vulkanContext);
    
    if (!std::ranges::all_of(shaderModules, &ShaderModule::IsValid))
    {
        return {};
    }
    
    return GraphicsPipelineBuilder(*vulkanContext)
        .SetDescriptorSetLayouts({ descriptors[GraphicsPipelineType::eVertex].second })
        .AddPushConstantRange({ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(gpu::PushConstants) })
        .SetShaderModules(std::move(shaderModules))
        .SetVertexData(SceneHelpers::GetVertexBindings(), SceneHelpers::GetVertexAttributes())
        .SetInputTopology(InputTopology::eTriangleList)
        .SetPolygonMode(PolygonMode::eFill)
        .SetCullMode(CullMode::eBack, false)
        .SetMultisampling(vulkanContext->GetDevice().GetProperties().maxSampleCount)
        .SetDepthState(true, true, VK_COMPARE_OP_GREATER_OR_EQUAL)
        .SetRenderPass(renderPass)
        .Build();
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
