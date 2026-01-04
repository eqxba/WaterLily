#include "Engine/Render/ForwardRenderer.hpp"

#include "Utils/Math.hpp"
#include "Engine/EventSystem.hpp"
#include "Engine/Scene/SceneHelpers.hpp"
#include "Engine/Render/RenderOptions.hpp"
#include "Engine/Render/Utils/MeshUtils.hpp"
#include "Engine/Render/Utils/ForwardUtils.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/RenderStages/DebugStage.hpp"
#include "Engine/Render/Vulkan/Image/ImageUtils.hpp"
#include "Engine/Render/Vulkan/Buffer/BufferUtils.hpp"
#include "Engine/Render/RenderStages/ForwardStage.hpp"
#include "Engine/Render/RenderStages/PrimitiveCullStage.hpp"

namespace ForwardRendererDetails
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

    static void CreateIndirectBuffers(RenderContext& renderContext, const VulkanContext& vulkanContext)
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

    static void CreateSceneBuffers(const RawScene& rawScene, RenderContext& renderContext, const VulkanContext& vulkanContext)
    {
        const std::span verticesSpan(rawScene.vertices);

        const BufferDescription vertexBufferDescription = {
            .size = verticesSpan.size_bytes(),
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
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
        
        // TODO: Create only when required
        const BufferDescription drawsVisibilityBufferDescription = {
            .size = draws.size() * sizeof(uint32_t),
            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

        renderContext.drawsVisibilityBuffer = Buffer(drawsVisibilityBufferDescription, false, vulkanContext);
        
        // TODO: We always create this one, but can skip if we implement compile time switch for debug features
        // And/or we can create it lazily
        const BufferDescription drawDebugDataBufferDescription = {
            .size = draws.size() * sizeof(uint32_t),
            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

        renderContext.drawsDebugDataBuffer = Buffer(drawDebugDataBufferDescription, false, vulkanContext);
    }
}

ForwardRenderer::ForwardRenderer(EventSystem& aEventSystem, const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
    , eventSystem{ &aEventSystem }
{
    InitRuntimeDefineGetters();
    
    CreateRenderPasses();

    primitiveCullStage = std::make_unique<PrimitiveCullStage>(*vulkanContext, renderContext);
    forwardStage = std::make_unique<ForwardStage>(*vulkanContext, renderContext);
    debugStage = std::make_unique<DebugStage>(*vulkanContext, renderContext);
    
    renderStages.push_back(primitiveCullStage.get());
    renderStages.push_back(forwardStage.get());
    renderStages.push_back(debugStage.get());
    
    CreateRenderTargets();
    CreateFramebuffers();

    eventSystem->Subscribe<ES::BeforeSwapchainRecreated>(this, &ForwardRenderer::OnBeforeSwapchainRecreated);
    eventSystem->Subscribe<ES::SwapchainRecreated>(this, &ForwardRenderer::OnSwapchainRecreated);
    eventSystem->Subscribe<ES::TryReloadShaders>(this, &ForwardRenderer::OnTryReloadShaders);
    eventSystem->Subscribe<ES::SceneOpened>(this, &ForwardRenderer::OnSceneOpen);
    eventSystem->Subscribe<ES::SceneClosed>(this, &ForwardRenderer::OnSceneClose);
    
    // Runtime defines
    eventSystem->Subscribe<RenderOptions::GraphicsPipelineTypeChanged>(this, &ForwardRenderer::OnTryReloadShaders);
    eventSystem->Subscribe<RenderOptions::VisualizeLodsChanged>(this, &ForwardRenderer::OnTryReloadShaders);
    eventSystem->Subscribe<RenderOptions::MsaaSampleCountChanged>(this, &ForwardRenderer::Reinitialize);
    eventSystem->Subscribe<RenderOptions::OcclusionCullingChanged>(this, &ForwardRenderer::Reinitialize);
}

ForwardRenderer::~ForwardRenderer()
{
    eventSystem->UnsubscribeAll(this);
    
    DestroyFramebuffers();
    DestroyRenderTargets();
}

void ForwardRenderer::Process(const Frame& frame, const float deltaSeconds)
{
    using namespace ForwardRendererDetails;

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
        const glm::vec4 frustumRight = Math::NormalizePlane(projectionT[3] - projectionT[0]);
        const glm::vec4 frustumTop = Math::NormalizePlane(projectionT[3] + projectionT[1]);

        cullData.frustumRightX = frustumRight.x;
        cullData.frustumRightZ = frustumRight.z;
        cullData.frustumTopY = frustumTop.y;
        cullData.frustumTopZ = frustumTop.z;
        cullData.near = camera.GetNear();
        
        renderContext.debugData.frustumCornersWorld = MeshUtils::GenerateFrustumCorners(camera);
    }
}

void ForwardRenderer::Render(const Frame& frame)
{
    using namespace VulkanUtils;
    using namespace ForwardRendererDetails;
    
    if (!scene)
    {
        return;
    }
    
    if (RenderOptions::Get().GetOcclusionCulling())
    {
        RenderWithOcclusionCulling(frame);
        return;
    }
    
    primitiveCullStage->Execute(frame);

    renderContext.renderPass.Begin(frame, renderContext.framebuffers[frame.swapchainImageIndex], GpuTimestamp::eFirstRenderPassBegin);
    forwardStage->Execute(frame);
    debugStage->Execute(frame);
    renderContext.renderPass.End(frame, GpuTimestamp::eFirstRenderPassEnd);
}

void ForwardRenderer::RenderWithOcclusionCulling(const Frame& frame)
{
    const bool msaaEnabled = VK_SAMPLE_COUNT_1_BIT != RenderOptions::Get().GetMsaaSampleCount();
    const bool freezeCamera = RenderOptions::Get().GetFreezeCamera();
    
    const VkFramebuffer firstPassFramebuffer = renderContext.firstPassFramebuffers[msaaEnabled ? 0 : frame.swapchainImageIndex];
    const VkFramebuffer secondPassFramebuffer = renderContext.secondPassFramebuffers[frame.swapchainImageIndex];
    
    primitiveCullStage->ExecuteFirstPass(frame);
        
    renderContext.firstRenderPass.Begin(frame, firstPassFramebuffer, GpuTimestamp::eFirstRenderPassBegin);
    forwardStage->Execute(frame);
    renderContext.firstRenderPass.End(frame, GpuTimestamp::eFirstRenderPassEnd);
    
    if (!freezeCamera)
    {
        primitiveCullStage->BuildDepthPyramid(frame);
    }
        
    if (!freezeCamera)
    {
        primitiveCullStage->ExecuteSecondPass(frame);
    }
    
    renderContext.secondRenderPass.Begin(frame, secondPassFramebuffer, GpuTimestamp::eSecondRenderPassBegin);
    
    if (!freezeCamera)
    {
        forwardStage->Execute(frame);
    }
    
    debugStage->Execute(frame); // TODO: Not a separate stage, just debug objects in the scene
    renderContext.secondRenderPass.End(frame, GpuTimestamp::eSecondRenderPassEnd);
    
    if (RenderOptions::Get().GetVisualizeDepth())
    {
        primitiveCullStage->VisualizeDepth(frame);
    }
}

void ForwardRenderer::InitRuntimeDefineGetters()
{
    using namespace gpu::defines;
    
    const DeviceProperties& deviceProperties = vulkanContext->GetDevice().GetProperties();
    std::unordered_map<std::string_view, std::function<int()>>& runtimeDefineGetters = renderContext.runtimeDefineGetters;
    
    runtimeDefineGetters.emplace(meshPipeline, []() { return RenderOptions::Get().GetGraphicsPipelineType() == GraphicsPipelineType::eMesh; });
    runtimeDefineGetters.emplace(drawIndirectCount, [=]() { return deviceProperties.drawIndirectCountSupported; });
    runtimeDefineGetters.emplace(visualizeLods, []() { return RenderOptions::Get().GetVisualizeLods(); });
}

void ForwardRenderer::CreateRenderPasses()
{
    const VkSampleCountFlagBits sampleCount = RenderOptions::Get().GetMsaaSampleCount();
    
    if (RenderOptions::Get().GetOcclusionCulling())
    {
        renderContext.firstRenderPass = ForwardUtils::CreateFirstOcclusionCullingRenderPass(sampleCount, *vulkanContext);
        renderContext.secondRenderPass = ForwardUtils::CreateSecondOcclusionCullingRenderPass(sampleCount, *vulkanContext);
    }
    else
    {
        renderContext.renderPass = ForwardUtils::CreateRenderPass(sampleCount, *vulkanContext);
    }
}

void ForwardRenderer::DestroyRenderPasses()
{
    renderContext.renderPass = {};
    renderContext.firstRenderPass = {};
    renderContext.secondRenderPass = {};
}

void ForwardRenderer::CreateRenderTargets()
{
    const RenderOptions& renderOptions = RenderOptions::Get();
    const VkSampleCountFlagBits sampleCount = renderOptions.GetMsaaSampleCount();
    
    // Create color target only when we use MSAA, otherwise render directly into swapchain
    if (sampleCount != VK_SAMPLE_COUNT_1_BIT)
    {
        renderContext.colorTarget = ForwardUtils::CreateColorTarget(sampleCount, *vulkanContext);
    }
    
    renderContext.depthTarget = ForwardUtils::CreateDepthTarget(sampleCount, *vulkanContext);
    
    // Create depth resolve target only when occlusion culling is enabled and we use MSAA, we'll need it for depth pyramid build
    if (renderOptions.GetOcclusionCulling() && sampleCount != VK_SAMPLE_COUNT_1_BIT)
    {
        renderContext.depthResolveTarget = ForwardUtils::CreateDepthResolveTarget(*vulkanContext);
    }
    
    std::ranges::for_each(renderStages, &RenderStage::CreateRenderTargetDependentResources);
}

void ForwardRenderer::DestroyRenderTargets()
{
    vulkanContext->GetDevice().WaitIdle();
    vulkanContext->GetDescriptorSetsManager().ResetDescriptors(DescriptorScope::eGlobal);
    
    std::ranges::for_each(renderStages, &RenderStage::DestroyRenderTargetDependentResources);
    
    renderContext.colorTarget = {};
    renderContext.depthTarget = {};
    renderContext.depthResolveTarget = {};
}

void ForwardRenderer::CreateFramebuffers()
{
    using namespace VulkanUtils;
    
    const Swapchain& swapchain = vulkanContext->GetSwapchain();
    const bool msaaEnabled = VK_SAMPLE_COUNT_1_BIT != RenderOptions::Get().GetMsaaSampleCount();
    
    RenderContext& rc = renderContext;
    
    if (const bool occlusionCulling = RenderOptions::Get().GetOcclusionCulling(); occlusionCulling)
    {
        if (msaaEnabled)
        {
            std::vector<VkImageView> firstPassAttachments = { rc.colorTarget.views[0], rc.depthTarget.views[0], rc.depthResolveTarget.views[0] };
            rc.firstPassFramebuffers.push_back(CreateFrameBuffer(rc.firstRenderPass, swapchain.GetExtent(), firstPassAttachments, *vulkanContext));
            
            std::vector<VkImageView> secondPassAttachments = { rc.colorTarget.views[0], VK_NULL_HANDLE /* for swapchain RT */, rc.depthTarget.views[0] };
            rc.secondPassFramebuffers = VulkanUtils::CreateFramebuffers(rc.secondRenderPass, swapchain, secondPassAttachments, *vulkanContext);
        }
        else
        {
            std::vector<VkImageView> attachments = { VK_NULL_HANDLE /* for swapchain RT */, rc.depthTarget.views[0] };
            
            rc.firstPassFramebuffers = VulkanUtils::CreateFramebuffers(rc.firstRenderPass, swapchain, attachments, *vulkanContext);
            rc.secondPassFramebuffers = VulkanUtils::CreateFramebuffers(rc.secondRenderPass, swapchain, attachments, *vulkanContext);
        }
    }
    else
    {
        if (msaaEnabled)
        {
            std::vector<VkImageView> attachments = { rc.colorTarget.views[0], VK_NULL_HANDLE /* for swapchain RT */, rc.depthTarget.views[0] };
            rc.framebuffers = VulkanUtils::CreateFramebuffers(rc.renderPass, swapchain, attachments, *vulkanContext);
        }
        else
        {
            std::vector<VkImageView> attachments = { VK_NULL_HANDLE /* for swapchain RT */, rc.depthTarget.views[0] };
            rc.framebuffers = VulkanUtils::CreateFramebuffers(rc.renderPass, swapchain, attachments, *vulkanContext);
        }
    }
}

void ForwardRenderer::DestroyFramebuffers()
{
    VulkanUtils::DestroyFramebuffers(renderContext.framebuffers, *vulkanContext);
    VulkanUtils::DestroyFramebuffers(renderContext.firstPassFramebuffers, *vulkanContext);
    VulkanUtils::DestroyFramebuffers(renderContext.secondPassFramebuffers, *vulkanContext);
}

void ForwardRenderer::Reinitialize()
{
    vulkanContext->GetDevice().WaitIdle();
    
    DestroyFramebuffers();
    DestroyRenderTargets();
    DestroyRenderPasses();
    
    CreateRenderPasses();
    CreateRenderTargets();
    CreateFramebuffers();
    
    OnTryReloadShaders();
}

void ForwardRenderer::OnBeforeSwapchainRecreated()
{    
    DestroyFramebuffers();
    DestroyRenderTargets();
}

void ForwardRenderer::OnSwapchainRecreated()
{
    CreateRenderTargets();
    CreateFramebuffers();
}

void ForwardRenderer::OnTryReloadShaders()
{
    if (std::ranges::all_of(renderStages, &RenderStage::TryRebuildPipelines))
    {
        vulkanContext->GetDevice().WaitIdle();
        vulkanContext->GetDescriptorSetsManager().ResetDescriptors(DescriptorScope::eSceneRenderer);
        
        std::ranges::for_each(renderStages, &RenderStage::ApplyRebuiltPipelines);
    }
}

void ForwardRenderer::OnSceneOpen(const ES::SceneOpened& event)
{
    using namespace BufferUtils;

    scene = &event.scene;

    if (vulkanContext->GetDevice().GetProperties().meshShadersSupported)
    {
        SceneHelpers::GenerateMeshlets(scene->GetRaw());
    }

    ForwardRendererDetails::CreateSceneBuffers(scene->GetRaw(), renderContext, *vulkanContext);
    ForwardRendererDetails::CreateIndirectBuffers(renderContext, *vulkanContext);
    
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
    
    vulkanContext->GetDevice().ExecuteOneTimeCommandBuffer([&](VkCommandBuffer cmd) {
        vkCmdFillBuffer(cmd, renderContext.drawsVisibilityBuffer, 0, VK_WHOLE_SIZE, 0);
        SynchronizationUtils::SetMemoryBarrier(cmd, Barriers::transferWriteToComputeRead);
    });
    
    std::ranges::for_each(renderStages, [&](RenderStage* stage) { stage->OnSceneOpen(*scene); });
}

void ForwardRenderer::OnSceneClose()
{
    vulkanContext->GetDevice().WaitIdle();
    vulkanContext->GetDescriptorSetsManager().ResetDescriptors(DescriptorScope::eSceneRenderer);

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
    
    std::ranges::for_each(renderStages, &RenderStage::OnSceneClose);
    
    scene = nullptr;
}
