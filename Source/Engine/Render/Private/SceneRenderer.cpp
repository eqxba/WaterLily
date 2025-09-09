#include "Engine/Render/SceneRenderer.hpp"

#include "Engine/EventSystem.hpp"
#include "Engine/Scene/SceneHelpers.hpp"
#include "Engine/Render/RenderOptions.hpp"
#include "Engine/Render/Utils/MeshUtils.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/RenderStages/DebugStage.hpp"
#include "Engine/Render/Vulkan/Buffer/BufferUtils.hpp"
#include "Engine/Render/RenderStages/ForwardStage.hpp"
#include "Engine/Render/RenderStages/PrimitiveCullStage.hpp"

namespace SceneRendererDetails
{
    static void SetSceneStats(const RawScene& rawScene, const std::vector<gpu::Draw>& draws)
    {
        uint64_t totalTriangles = 0;

        for (const gpu::Draw& draw : draws)
        {
            totalTriangles += rawScene.primitives[draw.primitiveIndex].lods[0].indexCount / 3;
        }

        Scene::SetTotalTriangles(totalTriangles);
    }

    static RenderPass CreateRenderPass(const VulkanContext& vulkanContext)
    {
        AttachmentDescription colorAttachmentDescription = {
            .format = vulkanContext.GetSwapchain().GetSurfaceFormat().format,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .actualLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .defaultClearValue = { .color = { { 0.73f, 0.95f, 1.0f, 1.0f } } } };
        
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
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .defaultClearValue = { .depthStencil = { 0.0f, 0 } } };
        
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

        const VkSampleCountFlagBits sampleCount = RenderOptions::Get().GetMsaaSampleCount();
        
        auto builder = RenderPassBuilder(vulkanContext)
            .SetBindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS)
            .SetMultisampling(sampleCount);
        
        // Use separate color attachment only when we use MSAA, otherwise render directly into swapchain
        // Swapchain images always have 1 sample and we do not provide resolve attachment in that case
        if (sampleCount == VK_SAMPLE_COUNT_1_BIT)
        {
            builder.AddColorAttachment(colorAttachmentDescription);
        }
        else
        {
            builder.AddColorAndResolveAttachments(colorAttachmentDescription, resolveAttachmentDescription);
        }
        
        return builder
            .AddDepthStencilAttachment(depthStencilAttachmentDescription)
            .SetPreviousBarriers(std::move(previousBarriers))
            .SetFollowingBarriers(std::move(followingBarriers))
            .Build();
    }

    void CreateIndirectBuffers(RenderContext& renderContext, const VulkanContext& vulkanContext)
    {
        const bool meshShadersSupported = vulkanContext.GetDevice().GetProperties().meshShadersSupported;

        const size_t largeEnoughCommandBuffer = gpu::primitiveCullMaxCommands * (meshShadersSupported
            ? std::max(sizeof(gpu::VkDrawIndexedIndirectCommand), sizeof(gpu::TaskCommand))
            : sizeof(gpu::VkDrawIndexedIndirectCommand));

        constexpr std::array<uint32_t, 3> commandCountValues = { 0, 1, 1 };
        const std::span commandCountSpan(commandCountValues);

        // We use it as buffer for vkCmdDrawMeshTasksIndirectEXT, so let's just always allocate 2 more uint32_t values
        // and set to them to 1 once on initialization bc we have 1-dimensional dispatch for tasks anyway
        const BufferDescription commandCountBufferDescription = {
            .size = commandCountSpan.size_bytes(),
            .usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

        renderContext.commandCountBuffer = Buffer(commandCountBufferDescription, true, commandCountSpan, vulkanContext);

        const BufferDescription commandBufferDescription = {
            .size = largeEnoughCommandBuffer,
            .usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

        renderContext.commandBuffer = Buffer(commandBufferDescription, false, vulkanContext);
    }

    void CreateSceneBuffers(const RawScene& rawScene, RenderContext& renderContext, const VulkanContext& vulkanContext)
    {
        const std::span verticesSpan(rawScene.vertices);

        const BufferDescription vertexBufferDescription = {
            .size = verticesSpan.size_bytes(),
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

        renderContext.vertexBuffer = Buffer(vertexBufferDescription, true, verticesSpan, vulkanContext);

        const std::span indicesSpan(rawScene.indices);

        const BufferDescription indexBufferDescription = {
            .size = indicesSpan.size_bytes(),
            .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

        renderContext.indexBuffer = Buffer(indexBufferDescription, true, indicesSpan, vulkanContext);

        if (!rawScene.meshletData.empty())
        {
            const std::span meshletDataSpan(rawScene.meshletData);

            const BufferDescription meshletDataBufferDescription = {
                .size = meshletDataSpan.size_bytes(),
                .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

            renderContext.meshletDataBuffer = Buffer(meshletDataBufferDescription, true, meshletDataSpan, vulkanContext);

            const std::span meshletSpan(rawScene.meshlets);

            const BufferDescription meshletBufferDescription = {
                .size = meshletSpan.size_bytes(),
                .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

            renderContext.meshletBuffer = Buffer(meshletBufferDescription, true, meshletSpan, vulkanContext);
        }

        const std::span primitiveSpan(rawScene.primitives);

        const BufferDescription primitiveBufferDescription = {
            .size = primitiveSpan.size_bytes(),
            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

        renderContext.primitiveBuffer = Buffer(primitiveBufferDescription, true, primitiveSpan, vulkanContext);
        
        const std::vector<gpu::Draw> draws = SceneHelpers::GenerateDraws(rawScene);
    
        RenderOptions& renderOptions = RenderOptions::Get();
        renderOptions.SetCurrentDrawCount(std::min(10'000u, static_cast<uint32_t>(draws.size())));
        renderOptions.SetMaxDrawCount(static_cast<uint32_t>(draws.size()));

        renderContext.globals.drawCount = renderOptions.GetCurrentDrawCount();

        SetSceneStats(rawScene, draws);

        const std::span drawSpan(draws);

        const BufferDescription drawBufferDescription = {
            .size = drawSpan.size_bytes(),
            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

        renderContext.drawBuffer = Buffer(drawBufferDescription, true, drawSpan, vulkanContext);
        
        // TODO: We always create this one, but can skip if we implement compile time switch for debug features
        // And/or we can create it lazily
        const BufferDescription drawDebugDataBufferDescription = {
            .size = draws.size() * sizeof(uint32_t),
            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

        renderContext.drawDebugDataBuffer = Buffer(drawDebugDataBufferDescription, false, vulkanContext);
    }

    static glm::vec4 NormalizePlane(const glm::vec4 plane)
    {
        return plane / glm::length(glm::vec3(plane));
    }
}

SceneRenderer::SceneRenderer(EventSystem& aEventSystem, const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
    , eventSystem{ &aEventSystem }
{
    renderContext.renderPass = SceneRendererDetails::CreateRenderPass(*vulkanContext);
    
    CreateRenderTargets();
    CreateFramebuffers();
    
    PrepareGlobalDefines();

    primitiveCullStage = std::make_unique<PrimitiveCullStage>(*vulkanContext, renderContext);
    forwardStage = std::make_unique<ForwardStage>(*vulkanContext, renderContext);
    debugStage = std::make_unique<DebugStage>(*vulkanContext, renderContext);
    
    renderStages.push_back(primitiveCullStage.get());
    renderStages.push_back(forwardStage.get());
    renderStages.push_back(debugStage.get());

    eventSystem->Subscribe<ES::BeforeSwapchainRecreated>(this, &SceneRenderer::OnBeforeSwapchainRecreated);
    eventSystem->Subscribe<ES::SwapchainRecreated>(this, &SceneRenderer::OnSwapchainRecreated);
    eventSystem->Subscribe<ES::TryReloadShaders>(this, &SceneRenderer::OnTryReloadShaders);
    eventSystem->Subscribe<ES::SceneOpened>(this, &SceneRenderer::OnSceneOpen);
    eventSystem->Subscribe<ES::SceneClosed>(this, &SceneRenderer::OnSceneClose);
    eventSystem->Subscribe<RenderOptions::GraphicsPipelineTypeChanged>(this, &SceneRenderer::OnGlobalDefinesChanged);
    eventSystem->Subscribe<RenderOptions::VisualizeLodsChanged>(this, &SceneRenderer::OnGlobalDefinesChanged);
    eventSystem->Subscribe<RenderOptions::MsaaSampleCountChanged>(this, &SceneRenderer::OnMsaaSampleCountChanged);
}

SceneRenderer::~SceneRenderer()
{
    eventSystem->UnsubscribeAll(this);
    
    DestroyFramebuffers();
    DestroyRenderTargets();
}

void SceneRenderer::Process(const Frame& frame, const float deltaSeconds)
{
    using namespace SceneRendererDetails;

    if (!scene) 
    {
        return;
    }
    
    const RenderOptions& renderOptions = RenderOptions::Get();
    
    const CameraComponent& camera = scene->GetCamera();
    const glm::mat4 projection = camera.GetProjectionMatrix();
    const VkExtent2D swapchainExtent = vulkanContext->GetSwapchain().GetExtent();

    renderContext.globals.view = camera.GetViewMatrix();
    renderContext.globals.projection = projection;
    renderContext.globals.drawCount = renderOptions.GetCurrentDrawCount();
    renderContext.globals.bUseLods = renderOptions.GetUseLods();
    renderContext.globals.lodTarget = glm::tan(camera.GetVerticalFov() / 2.0f) 
        * 2.0f / static_cast<float>(swapchainExtent.height); // 1px in primitive space

    if (!renderOptions.GetFreezeCamera())
    {
        gpu::CullData& cullData = renderContext.globals.cullData;

        cullData.view = renderContext.globals.view;

        const glm::mat4 projectionT = transpose(projection);
        const glm::vec4 frustumRight = NormalizePlane(projectionT[3] - projectionT[0]);
        const glm::vec4 frustumTop = NormalizePlane(projectionT[3] + projectionT[1]);

        cullData.frustumRightX = frustumRight.x;
        cullData.frustumRightZ = frustumRight.z;
        cullData.frustumTopY = frustumTop.y;
        cullData.frustumTopZ = frustumTop.z;
        cullData.near = -camera.GetNear();
        
        renderContext.debugData.frustumCornersWorld = MeshUtils::GenerateFrustumCorners(camera);
    }
}

void SceneRenderer::Render(const Frame& frame)
{
    using namespace VulkanUtils;
    using namespace SceneRendererDetails;
    
    if (!scene)
    {
        return;
    }
    
    primitiveCullStage->Execute(frame);
    
    const VkRenderPassBeginInfo beginInfo = renderContext.renderPass.GetBeginInfo(renderContext.framebuffers[frame.swapchainImageIndex]);
    vkCmdBeginRenderPass(frame.commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    forwardStage->Execute(frame);
    debugStage->Execute(frame);
    
    vkCmdEndRenderPass(frame.commandBuffer);
}

void SceneRenderer::CreateRenderTargets()
{
    const Swapchain& swapchain = vulkanContext->GetSwapchain();
    const VkExtent2D swapchainExtent = swapchain.GetExtent();
    const VkSampleCountFlagBits sampleCount = RenderOptions::Get().GetMsaaSampleCount();

    // Create color target only when we use MSAA, otherwise render directly into swapchain
    if (sampleCount != VK_SAMPLE_COUNT_1_BIT)
    {
        ImageDescription colorTargetDescription = {
            .extent = { swapchainExtent.width, swapchainExtent.height, 1 },
            .mipLevelsCount = 1,
            .samples = sampleCount,
            .format = swapchain.GetSurfaceFormat().format,
            .usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
        
        renderContext.colorTarget = RenderTarget(colorTargetDescription, VK_IMAGE_ASPECT_COLOR_BIT, *vulkanContext);
    }
    
    ImageDescription depthTargetDescription = {
        .extent = { swapchainExtent.width, swapchainExtent.height, 1 },
        .mipLevelsCount = 1,
        .samples = sampleCount,
        .format = VulkanConfig::depthImageFormat,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
    
    renderContext.depthTarget = RenderTarget(depthTargetDescription, VK_IMAGE_ASPECT_DEPTH_BIT, *vulkanContext);
}

void SceneRenderer::DestroyRenderTargets()
{
    renderContext.colorTarget = {};
    renderContext.depthTarget = {};
}

void SceneRenderer::CreateFramebuffers()
{
    const Swapchain& swapchain = vulkanContext->GetSwapchain();
    const bool msaaEnabled = VK_SAMPLE_COUNT_1_BIT != RenderOptions::Get().GetMsaaSampleCount();

    std::vector<VkImageView> attachments = { VK_NULL_HANDLE /* for swapchain RT */, renderContext.depthTarget.view };
    
    if (msaaEnabled)
    {
        attachments.insert(attachments.begin(), renderContext.colorTarget.view);
    }
    
    const auto createFrameBuffer = [&](const RenderTarget& target) {
        attachments[msaaEnabled ? 1 : 0] = target.view;
        return VulkanUtils::CreateFrameBuffer(renderContext.renderPass, swapchain.GetExtent(), attachments, *vulkanContext);
    };

    // See CreateRenderPass for attachments usage details
    std::ranges::transform(swapchain.GetRenderTargets(), std::back_inserter(renderContext.framebuffers), createFrameBuffer);
}

void SceneRenderer::DestroyFramebuffers()
{
    VulkanUtils::DestroyFramebuffers(renderContext.framebuffers, *vulkanContext);
}

void SceneRenderer::PrepareGlobalDefines()
{
    const RenderOptions& renderOptions = RenderOptions::Get();
    const DeviceProperties& deviceProperties = vulkanContext->GetDevice().GetProperties();
    
    // TODO: Defines storage structures
    renderContext.visualizeLods = renderOptions.GetVisualizeLods();
    renderContext.graphicsPipelineType = renderOptions.GetGraphicsPipelineType();
    
    renderContext.globalDefines.clear();
    
    renderContext.globalDefines.emplace_back(gpu::defines::meshPipeline, renderContext.graphicsPipelineType == GraphicsPipelineType::eMesh ? "1" : "0");
    renderContext.globalDefines.emplace_back(gpu::defines::drawIndirectCount, deviceProperties.drawIndirectCountSupported ? "1" : "0");
    renderContext.globalDefines.emplace_back(gpu::defines::visualizeLods, renderContext.visualizeLods ? "1" : "0");
}

void SceneRenderer::OnBeforeSwapchainRecreated()
{
    DestroyFramebuffers();
    DestroyRenderTargets();
}

void SceneRenderer::OnSwapchainRecreated()
{
    CreateRenderTargets();
    CreateFramebuffers();
}

void SceneRenderer::OnTryReloadShaders()
{
    if (std::ranges::all_of(renderStages, &RenderStage::TryReloadShaders))
    {
        vulkanContext->GetDevice().WaitIdle();
        vulkanContext->GetDescriptorSetsManager().ResetDescriptors(DescriptorScope::eSceneRenderer);
        
        std::ranges::for_each(renderStages, &RenderStage::ApplyReloadedShaders);
    }
}

void SceneRenderer::OnSceneOpen(const ES::SceneOpened& event)
{
    using namespace BufferUtils;

    scene = &event.scene;

    if (vulkanContext->GetDevice().GetProperties().meshShadersSupported)
    {
        SceneHelpers::GenerateMeshlets(scene->GetRaw());
    }

    SceneRendererDetails::CreateSceneBuffers(scene->GetRaw(), renderContext, *vulkanContext);
    SceneRendererDetails::CreateIndirectBuffers(renderContext, *vulkanContext);
    
    UploadFromStagingBuffers(*vulkanContext, Barriers::transferWriteToComputeRead, /* destroyStagingBuffers */ true,
        renderContext.vertexBuffer,
        renderContext.indexBuffer,
        renderContext.primitiveBuffer,
        renderContext.drawBuffer,
        renderContext.commandCountBuffer);
    
    if (renderContext.meshletDataBuffer.IsValid())
    {
        UploadFromStagingBuffers(*vulkanContext, Barriers::transferWriteToComputeRead, /* destroyStagingBuffers */ true,
            renderContext.meshletDataBuffer,
            renderContext.meshletBuffer);
    }
    
    std::ranges::for_each(renderStages, [&](RenderStage* stage) { stage->Prepare(*scene); });
}

void SceneRenderer::OnSceneClose()
{
    vulkanContext->GetDevice().WaitIdle();

    renderContext.vertexBuffer = {};
    renderContext.indexBuffer = {};

    if (renderContext.meshletDataBuffer.IsValid())
    {
        renderContext.meshletDataBuffer = {};
        renderContext.meshletBuffer = {};
    }

    renderContext.primitiveBuffer = {};
    renderContext.drawBuffer = {};
    renderContext.commandCountBuffer = {};
    renderContext.commandBuffer = {};

    vulkanContext->GetDescriptorSetsManager().ResetDescriptors(DescriptorScope::eSceneRenderer);
    
    std::ranges::for_each(renderStages, &RenderStage::OnSceneClose);
    
    scene = nullptr;
}

void SceneRenderer::OnGlobalDefinesChanged()
{
    PrepareGlobalDefines();
    eventSystem->Fire<ES::TryReloadShaders>();
}

void SceneRenderer::OnMsaaSampleCountChanged()
{
    vulkanContext->GetDevice().WaitIdle();
    
    renderContext.renderPass = SceneRendererDetails::CreateRenderPass(*vulkanContext);
    
    DestroyFramebuffers();
    DestroyRenderTargets();
    
    CreateRenderTargets();
    CreateFramebuffers();
    
    std::ranges::for_each(renderStages, &RenderStage::RecreatePipelinesAndDescriptors);
}
