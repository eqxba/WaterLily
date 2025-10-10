#include "Engine/Render/SceneRenderer.hpp"

#include "Engine/EventSystem.hpp"
#include "Engine/Scene/SceneHelpers.hpp"
#include "Engine/Render/RenderOptions.hpp"
#include "Engine/Render/Utils/MeshUtils.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/RenderStages/DebugStage.hpp"
#include "Engine/Render/Vulkan/Image/ImageUtils.hpp"
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

    static RenderPass CreateFirstRenderPass(const VulkanContext& vulkanContext)
    {
        const VkSampleCountFlagBits sampleCount = RenderOptions::Get().GetMsaaSampleCount();
        const bool singleSample = sampleCount == VK_SAMPLE_COUNT_1_BIT; // Rendering into swapchain directly
        
        AttachmentDescription colorAttachmentDescription = {
            .format = vulkanContext.GetSwapchain().GetSurfaceFormat().format,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .initialLayout = singleSample ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .actualLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .defaultClearValue = { .color = { { 0.73f, 0.95f, 1.0f, 1.0f } } } };
        
        AttachmentDescription depthStencilAttachmentDescription = {
            .format = VulkanConfig::depthImageFormat,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .actualLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .finalLayout = singleSample ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .defaultClearValue = { .depthStencil = { 0.0f, 0 } } };
        
        AttachmentDescription depthStencilResolveAttachmentDescription = {
            .format = VulkanConfig::depthImageFormat,
            .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE, // We're building depth pyramid from it
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .actualLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }; // So we'll need to read from it
        
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
        
        auto builder = RenderPassBuilder(vulkanContext)
            .SetBindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS)
            .SetMultisampling(sampleCount)
            .AddColorAttachment(colorAttachmentDescription);
        
        if (singleSample)
        {
            builder.AddDepthStencilAttachment(depthStencilAttachmentDescription);
        }
        else
        {
            builder.AddDepthStencilAndResolveAttachments(depthStencilAttachmentDescription, depthStencilResolveAttachmentDescription);
        }
        
        return builder
            .SetPreviousBarriers(std::move(previousBarriers))
            .Build();
    }

    static RenderPass CreateSecondRenderPass(const VulkanContext& vulkanContext)
    {
        const VkSampleCountFlagBits sampleCount = RenderOptions::Get().GetMsaaSampleCount();
        const bool singleSample = sampleCount == VK_SAMPLE_COUNT_1_BIT;
        
        AttachmentDescription colorAttachmentDescription = {
            .format = vulkanContext.GetSwapchain().GetSurfaceFormat().format,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD, // Load in the 2nd pass
            .storeOp = singleSample ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = singleSample ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
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
        
        auto builder = RenderPassBuilder(vulkanContext)
            .SetBindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS)
            .SetMultisampling(sampleCount);
        
        // Render directly into swapchain, use separate color + resolve pair only when we use MSAA (swapchain RT is always 1 sample)
        if (singleSample)
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

    static glm::vec4 NormalizePlane(const glm::vec4 plane)
    {
        return plane / glm::length(glm::vec3(plane));
    }
}

SceneRenderer::SceneRenderer(EventSystem& aEventSystem, const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
    , eventSystem{ &aEventSystem }
{
    renderContext.firstRenderPass = SceneRendererDetails::CreateFirstRenderPass(*vulkanContext);
    renderContext.secondRenderPass = SceneRendererDetails::CreateSecondRenderPass(*vulkanContext);
    
    InitRuntimeDefineGetters();

    primitiveCullStage = std::make_unique<PrimitiveCullStage>(*vulkanContext, renderContext);
    forwardStage = std::make_unique<ForwardStage>(*vulkanContext, renderContext);
    debugStage = std::make_unique<DebugStage>(*vulkanContext, renderContext);
    
    renderStages.push_back(primitiveCullStage.get());
    renderStages.push_back(forwardStage.get());
    renderStages.push_back(debugStage.get());
    
    CreateRenderTargets();
    CreateFramebuffers();

    eventSystem->Subscribe<ES::BeforeSwapchainRecreated>(this, &SceneRenderer::OnBeforeSwapchainRecreated);
    eventSystem->Subscribe<ES::SwapchainRecreated>(this, &SceneRenderer::OnSwapchainRecreated);
    eventSystem->Subscribe<ES::TryReloadShaders>(this, &SceneRenderer::OnTryReloadShaders);
    eventSystem->Subscribe<ES::SceneOpened>(this, &SceneRenderer::OnSceneOpen);
    eventSystem->Subscribe<ES::SceneClosed>(this, &SceneRenderer::OnSceneClose);
    
    // Runtime defines
    eventSystem->Subscribe<RenderOptions::GraphicsPipelineTypeChanged>(this, &SceneRenderer::OnTryReloadShaders);
    eventSystem->Subscribe<RenderOptions::VisualizeLodsChanged>(this, &SceneRenderer::OnTryReloadShaders);
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
        cullData.near = camera.GetNear();
        
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
    
    const bool msaaEnabled = VK_SAMPLE_COUNT_1_BIT != RenderOptions::Get().GetMsaaSampleCount();
    const bool freezeCamera = RenderOptions::Get().GetFreezeCamera();
    
    const VkFramebuffer firstPassFramebuffer = renderContext.firstPassFramebuffers[msaaEnabled ? 0 : frame.swapchainImageIndex];
    const VkFramebuffer secondPassFramebuffer = renderContext.secondPassFramebuffers[frame.swapchainImageIndex];
    
    StatsUtils::WriteTimestamp(frame.commandBuffer, frame.queryPools.timestamps, GpuTimestamp::eFirstPassBegin);
    
    primitiveCullStage->ExecuteFirstPass(frame);
    
    renderContext.firstRenderPass.Begin(frame.commandBuffer, firstPassFramebuffer);
    forwardStage->Execute(frame);
    renderContext.firstRenderPass.End(frame.commandBuffer);
    
    StatsUtils::WriteTimestamp(frame.commandBuffer, frame.queryPools.timestamps, GpuTimestamp::eFirstPassEnd);
    
    if (!freezeCamera)
    {
        primitiveCullStage->BuildDepthPyramid(frame);
    }
        
    StatsUtils::WriteTimestamp(frame.commandBuffer, frame.queryPools.timestamps, GpuTimestamp::eDepthPyramidEnd);
        
    if (!freezeCamera)
    {
        primitiveCullStage->ExecuteSecondPass(frame);
    }
    
    renderContext.secondRenderPass.Begin(frame.commandBuffer, secondPassFramebuffer);
    
    if (!freezeCamera)
    {
        forwardStage->Execute(frame);
    }
    
    debugStage->Execute(frame); // TODO: Not a separate stage, just debug objects in the scene
    renderContext.secondRenderPass.End(frame.commandBuffer);
    
    StatsUtils::WriteTimestamp(frame.commandBuffer, frame.queryPools.timestamps, GpuTimestamp::eSecondPassEnd);
    
    if (RenderOptions::Get().GetVisualizeDepth())
    {
        primitiveCullStage->VisualizeDepth(frame);
    }
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
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, // rEMOVE SAMPLeD !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
    
    renderContext.depthTarget = RenderTarget(depthTargetDescription, VK_IMAGE_ASPECT_DEPTH_BIT, *vulkanContext);
    
    // Create depth resolve target only when we use MSAA, we'll need it for depth pyramid build
    if (sampleCount != VK_SAMPLE_COUNT_1_BIT)
    {
        ImageDescription depthResolveTargetDescription = {
            .extent = { swapchainExtent.width, swapchainExtent.height, 1 },
            .mipLevelsCount = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .format = VulkanConfig::depthImageFormat,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
        
        renderContext.depthResolveTarget = RenderTarget(depthResolveTargetDescription, VK_IMAGE_ASPECT_DEPTH_BIT, *vulkanContext);
    }
    
    vulkanContext->GetDevice().ExecuteOneTimeCommandBuffer([&](VkCommandBuffer cmd) {
        if (sampleCount != VK_SAMPLE_COUNT_1_BIT)
        {
            ImageUtils::TransitionLayout(cmd, renderContext.colorTarget, LayoutTransitions::undefinedToColorAttachmentOptimal, {
                .srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, .srcAccessMask = VK_ACCESS_NONE,
                .dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT });
        }
        
        ImageUtils::TransitionLayout(cmd, renderContext.depthTarget, LayoutTransitions::undefinedToDepthStencilAttachmentOptimal, {
            .srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, .srcAccessMask = VK_ACCESS_NONE,
            .dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT });
        
        if (sampleCount != VK_SAMPLE_COUNT_1_BIT)
        {
            ImageUtils::TransitionLayout(cmd, renderContext.depthResolveTarget, LayoutTransitions::undefinedToShaderReadOnlyOptimal, {
                .srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, .srcAccessMask = VK_ACCESS_NONE,
                .dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT });
        }
    });
    
    std::ranges::for_each(renderStages, &RenderStage::CreateRenderTargetDependentResources);
}

void SceneRenderer::DestroyRenderTargets()
{
    vulkanContext->GetDevice().WaitIdle();
    vulkanContext->GetDescriptorSetsManager().ResetDescriptors(DescriptorScope::eGlobal);
    
    std::ranges::for_each(renderStages, &RenderStage::DestroyRenderTargetDependentResources);
    
    renderContext.colorTarget = {};
    renderContext.depthTarget = {};
    renderContext.depthResolveTarget = {};
}

// TODO: This is too unereadable at this point, split into 2 functions
void SceneRenderer::CreateFramebuffers()
{
    using namespace VulkanUtils;
    
    const Swapchain& swapchain = vulkanContext->GetSwapchain();
    const bool msaaEnabled = VK_SAMPLE_COUNT_1_BIT != RenderOptions::Get().GetMsaaSampleCount();

    std::vector<VkImageView> attachments = { VK_NULL_HANDLE /* for swapchain RT */, renderContext.depthTarget.views[0] };
    
    const auto createFrameBuffer = [&](const RenderTarget& target) {
        attachments[msaaEnabled ? 1 : 0] = target.views[0];
        return VulkanUtils::CreateFrameBuffer(renderContext.secondRenderPass, swapchain.GetExtent(), attachments, *vulkanContext);
    };
    
    // First pass framebuffers
    if (msaaEnabled)
    {
        attachments[0] = renderContext.colorTarget.views[0];
        attachments.push_back(renderContext.depthResolveTarget.views[0]);
        renderContext.firstPassFramebuffers.push_back(CreateFrameBuffer(renderContext.firstRenderPass, swapchain.GetExtent(), attachments, *vulkanContext));
        attachments.pop_back();
    }
    else
    {
        std::ranges::transform(swapchain.GetRenderTargets(), std::back_inserter(renderContext.firstPassFramebuffers), createFrameBuffer);
    }
    
    // Second pass framebuffers
    if (msaaEnabled)
    {
        attachments.insert(attachments.begin(), renderContext.colorTarget.views[0]);
    }

    std::ranges::transform(swapchain.GetRenderTargets(), std::back_inserter(renderContext.secondPassFramebuffers), createFrameBuffer);
}

void SceneRenderer::DestroyFramebuffers()
{
    VulkanUtils::DestroyFramebuffers(renderContext.firstPassFramebuffers, *vulkanContext);
    VulkanUtils::DestroyFramebuffers(renderContext.secondPassFramebuffers, *vulkanContext);
}

void SceneRenderer::InitRuntimeDefineGetters()
{
    using namespace gpu::defines;
    
    const DeviceProperties& deviceProperties = vulkanContext->GetDevice().GetProperties();
    std::unordered_map<std::string_view, std::function<int()>>& runtimeDefineGetters = renderContext.runtimeDefineGetters;
    
    runtimeDefineGetters.emplace(meshPipeline, []() { return RenderOptions::Get().GetGraphicsPipelineType() == GraphicsPipelineType::eMesh; });
    runtimeDefineGetters.emplace(drawIndirectCount, [=]() { return deviceProperties.drawIndirectCountSupported; });
    runtimeDefineGetters.emplace(visualizeLods, []() { return RenderOptions::Get().GetVisualizeLods(); });
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
    if (std::ranges::all_of(renderStages, &RenderStage::TryRebuildPipelines))
    {
        vulkanContext->GetDevice().WaitIdle();
        vulkanContext->GetDescriptorSetsManager().ResetDescriptors(DescriptorScope::eSceneRenderer);
        
        std::ranges::for_each(renderStages, &RenderStage::ApplyRebuiltPipelines);
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
    
    vulkanContext->GetDevice().ExecuteOneTimeCommandBuffer([&](VkCommandBuffer cmd) {
        vkCmdFillBuffer(cmd, renderContext.drawsVisibilityBuffer, 0, VK_WHOLE_SIZE, 0);
        SynchronizationUtils::SetMemoryBarrier(cmd, Barriers::transferWriteToComputeRead);
    });
    
    std::ranges::for_each(renderStages, [&](RenderStage* stage) { stage->OnSceneOpen(*scene); });
}

void SceneRenderer::OnSceneClose()
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

void SceneRenderer::OnMsaaSampleCountChanged()
{
    vulkanContext->GetDevice().WaitIdle();
    
    renderContext.firstRenderPass = SceneRendererDetails::CreateFirstRenderPass(*vulkanContext);
    renderContext.secondRenderPass = SceneRendererDetails::CreateSecondRenderPass(*vulkanContext);
    
    DestroyFramebuffers();
    DestroyRenderTargets();
    
    CreateRenderTargets();
    CreateFramebuffers();
    
    OnTryReloadShaders();
}
