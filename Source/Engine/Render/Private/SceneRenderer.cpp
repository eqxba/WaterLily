#include "Engine/Render/SceneRenderer.hpp"

#include "Engine/EventSystem.hpp"
#include "Engine/Scene/SceneHelpers.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Buffer/BufferUtils.hpp"
#include "Engine/Render/RenderStages/ForwardStage.hpp"
#include "Engine/Render/RenderStages/PrimitiveCullStage.hpp"

namespace SceneRendererDetails
{
    static constexpr PipelineBarrier uploadBarrier = {
        .srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT };

    void CreateSceneBuffers(const RawScene& rawScene, RenderContext& renderContext, const VulkanContext& vulkanContext)
    {
        using namespace BufferUtils;

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

        vulkanContext.GetDevice().ExecuteOneTimeCommandBuffer([&](const VkCommandBuffer cmd) {
            CopyBufferToBuffer(cmd, renderContext.vertexBuffer.GetStagingBuffer(), renderContext.vertexBuffer);
            CopyBufferToBuffer(cmd, renderContext.indexBuffer.GetStagingBuffer(), renderContext.indexBuffer);
            CopyBufferToBuffer(cmd, renderContext.primitiveBuffer.GetStagingBuffer(), renderContext.primitiveBuffer);
            CopyBufferToBuffer(cmd, renderContext.drawBuffer.GetStagingBuffer(), renderContext.drawBuffer);

            SynchronizationUtils::SetMemoryBarrier(cmd, uploadBarrier);
        });

        renderContext.vertexBuffer.DestroyStagingBuffer();
        renderContext.indexBuffer.DestroyStagingBuffer();
        renderContext.primitiveBuffer.DestroyStagingBuffer();
        renderContext.drawBuffer.DestroyStagingBuffer();
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
    scene = &event.scene;

    SceneRendererDetails::CreateSceneBuffers(scene->GetRaw(), renderContext, *vulkanContext);   
    
    primitiveCullStage->Prepare(*scene);
    forwardStage->Prepare(*scene);
}

void SceneRenderer::OnSceneClose(const ES::SceneClosed& event)
{
    // TODO: Actually we have to reset context here (do we actually have to?)
    vulkanContext->GetDevice().WaitIdle();
    vulkanContext->GetDescriptorSetsManager().ResetDescriptors(DescriptorScope::eSceneRenderer);
    
    scene = nullptr;
}
