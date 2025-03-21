#include "Engine/Render/SceneRenderer.hpp"

#include "Engine/EventSystem.hpp"
#include "Engine/Scene/SceneHelpers.hpp"
#include "Engine/Render/RenderOptions.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Buffer/BufferUtils.hpp"
#include "Engine/Render/RenderStages/ForwardStage.hpp"
#include "Engine/Render/RenderStages/PrimitiveCullStage.hpp"

namespace SceneRendererDetails
{
    void CreateIndirectBuffers(RenderContext& renderContext, const VulkanContext& vulkanContext)
    {
        const bool meshShadersSupported = vulkanContext.GetDevice().GetProperties().meshShadersSupported;

        const size_t largeEnoughCommandBuffer = gpu::primitiveCullMaxCommands * (meshShadersSupported
            ? std::max(sizeof(gpu::IndirectCommand), sizeof(gpu::TaskCommand))
            : sizeof(gpu::IndirectCommand));

        const std::vector<uint32_t> commandCountValues = { 0, 1, 1 };
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
        const std::span verticesSpan(std::as_const(rawScene.vertices));

        const BufferDescription vertexBufferDescription = {
            .size = verticesSpan.size_bytes(),
            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

        renderContext.vertexBuffer = Buffer(vertexBufferDescription, true, verticesSpan, vulkanContext);

        const std::span indicesSpan(std::as_const(rawScene.indices));

        const BufferDescription indexBufferDescription = {
            .size = indicesSpan.size_bytes(),
            .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

        renderContext.indexBuffer = Buffer(indexBufferDescription, true, indicesSpan, vulkanContext);

        if (!rawScene.meshletData.empty())
        {
            const std::span meshletDataSpan(std::as_const(rawScene.meshletData));

            const BufferDescription meshletDataBufferDescription = {
                .size = meshletDataSpan.size_bytes(),
                .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

            renderContext.meshletDataBuffer = Buffer(meshletDataBufferDescription, true, meshletDataSpan, vulkanContext);

            const std::span meshletSpan(std::as_const(rawScene.meshlets));

            const BufferDescription meshletBufferDescription = {
                .size = meshletSpan.size_bytes(),
                .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

            renderContext.meshletBuffer = Buffer(meshletBufferDescription, true, meshletSpan, vulkanContext);
        }

        const std::span primitiveSpan(std::as_const(rawScene.primitives));

        const BufferDescription primitiveBufferDescription = {
            .size = primitiveSpan.size_bytes(),
            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

        renderContext.primitiveBuffer = Buffer(primitiveBufferDescription, true, primitiveSpan, vulkanContext);

        const std::vector<gpu::Draw> draws = SceneHelpers::GenerateDraws(rawScene);
        renderContext.globals.drawCount = static_cast<uint32_t>(draws.size());

        const std::span drawSpan(std::as_const(draws));

        const BufferDescription drawBufferDescription = {
            .size = drawSpan.size_bytes(),
            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

        renderContext.drawBuffer = Buffer(drawBufferDescription, true, drawSpan, vulkanContext);
    }
}

SceneRenderer::SceneRenderer(EventSystem& aEventSystem, const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
    , eventSystem{ &aEventSystem }
{
    using namespace SceneRendererDetails;

    CreateRenderTargets();

    primitiveCullStage = std::make_unique<PrimitiveCullStage>(*vulkanContext, renderContext);
    forwardStage = std::make_unique<ForwardStage>(*vulkanContext, renderContext);

    eventSystem->Subscribe<ES::BeforeSwapchainRecreated>(this, &SceneRenderer::OnBeforeSwapchainRecreated);
    eventSystem->Subscribe<ES::SwapchainRecreated>(this, &SceneRenderer::OnSwapchainRecreated);
    eventSystem->Subscribe<ES::TryReloadShaders>(this, &SceneRenderer::OnTryReloadShaders);
    eventSystem->Subscribe<ES::SceneOpened>(this, &SceneRenderer::OnSceneOpen);
    eventSystem->Subscribe<ES::SceneClosed>(this, &SceneRenderer::OnSceneClose);
}

SceneRenderer::~SceneRenderer()
{
    eventSystem->UnsubscribeAll(this);
    
    DestroyRenderTargets();
}

void SceneRenderer::Process(const float deltaSeconds)
{
    if (!scene) 
    {
        return;
    }

    const CameraComponent& camera = scene->GetCamera();

    renderContext.globals.view = camera.GetViewMatrix();
    renderContext.globals.projection = camera.GetProjectionMatrix();
    renderContext.globals.viewPos = camera.GetPosition();
    renderContext.globals.bMeshPipeline = RenderOptions::Get().GetGraphicsPipelineType() == GraphicsPipelineType::eMesh;
}

void SceneRenderer::Render(const Frame& frame)
{
    if (!scene)
    {
        return;
    }

    primitiveCullStage->Execute(frame);
    forwardStage->Execute(frame);
}

void SceneRenderer::CreateRenderTargets()
{
    const Swapchain& swapchain = vulkanContext->GetSwapchain();
    const VkExtent2D swapchainExtent = swapchain.GetExtent();

    // Render targets
    ImageDescription colorTargetDescription = {
        .extent = { swapchainExtent.width, swapchainExtent.height, 1 },
        .mipLevelsCount = 1,
        .samples = vulkanContext->GetDevice().GetProperties().maxSampleCount,
        .format = swapchain.GetSurfaceFormat().format,
        .usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
    
    ImageDescription depthTargetDescription = {
        .extent = { swapchainExtent.width, swapchainExtent.height, 1 },
        .mipLevelsCount = 1,
        .samples = vulkanContext->GetDevice().GetProperties().maxSampleCount,
        .format = VulkanConfig::depthImageFormat,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
    
    renderContext.colorTarget = RenderTarget(colorTargetDescription, VK_IMAGE_ASPECT_COLOR_BIT, *vulkanContext);
    renderContext.depthTarget = RenderTarget(depthTargetDescription, VK_IMAGE_ASPECT_DEPTH_BIT, *vulkanContext);
}

void SceneRenderer::DestroyRenderTargets()
{
    renderContext.colorTarget = {};
    renderContext.depthTarget = {};
}

void SceneRenderer::OnBeforeSwapchainRecreated(const ES::BeforeSwapchainRecreated& event)
{
    DestroyRenderTargets();
}

void SceneRenderer::OnSwapchainRecreated(const ES::SwapchainRecreated& event)
{
    CreateRenderTargets();

    primitiveCullStage->RecreateFramebuffers();
    forwardStage->RecreateFramebuffers();
}

void SceneRenderer::OnTryReloadShaders(const ES::TryReloadShaders& event)
{
    primitiveCullStage->TryReloadShaders();
    forwardStage->TryReloadShaders();
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

    vulkanContext->GetDevice().ExecuteOneTimeCommandBuffer([&](const VkCommandBuffer cmd) {
        CopyBufferToBuffer(cmd, renderContext.vertexBuffer.GetStagingBuffer(), renderContext.vertexBuffer);
        CopyBufferToBuffer(cmd, renderContext.indexBuffer.GetStagingBuffer(), renderContext.indexBuffer);

        if (renderContext.meshletDataBuffer.IsValid())
        {
            CopyBufferToBuffer(cmd, renderContext.meshletDataBuffer.GetStagingBuffer(), renderContext.meshletDataBuffer);
            CopyBufferToBuffer(cmd, renderContext.meshletBuffer.GetStagingBuffer(), renderContext.meshletBuffer);
        }

        CopyBufferToBuffer(cmd, renderContext.primitiveBuffer.GetStagingBuffer(), renderContext.primitiveBuffer);
        CopyBufferToBuffer(cmd, renderContext.drawBuffer.GetStagingBuffer(), renderContext.drawBuffer);
        CopyBufferToBuffer(cmd, renderContext.commandCountBuffer.GetStagingBuffer(), renderContext.commandCountBuffer);

        SynchronizationUtils::SetMemoryBarrier(cmd, Barriers::transferWriteToComputeRead);
    });

    renderContext.vertexBuffer.DestroyStagingBuffer();
    renderContext.indexBuffer.DestroyStagingBuffer();

    if (renderContext.meshletDataBuffer.IsValid())
    {
        renderContext.meshletDataBuffer.DestroyStagingBuffer();
        renderContext.meshletBuffer.DestroyStagingBuffer();
    }

    renderContext.primitiveBuffer.DestroyStagingBuffer();
    renderContext.drawBuffer.DestroyStagingBuffer();
    renderContext.commandCountBuffer.DestroyStagingBuffer();
    
    primitiveCullStage->Prepare(*scene);
    forwardStage->Prepare(*scene);
}

void SceneRenderer::OnSceneClose(const ES::SceneClosed& event)
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
    
    scene = nullptr;
}
