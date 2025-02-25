#include "Engine/Systems/RenderSystem.hpp"

#include "Engine/Engine.hpp"
#include "Engine/EventSystem.hpp"
#include "Engine/Window.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/UI/UiRenderer.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/Render/Resources/Image.hpp"
#include "Engine/Render/Resources/ImageView.hpp"
#include "Shaders/Common.h"

#include <glm/gtc/matrix_transform.hpp>

namespace RenderSystemDetails
{
    static constexpr std::string_view vertexShaderPath = "~/Shaders/shader.vert";
    static constexpr std::string_view fragmentShaderPath = "~/Shaders/shader.frag";

    static std::vector<ShaderModule> GetShaderModules(const ShaderManager& shaderManager)
    {
        std::vector<ShaderModule> shaderModules;
        shaderModules.reserve(2);

        shaderModules.emplace_back(shaderManager.CreateShaderModule(FilePath(vertexShaderPath), ShaderType::eVertex));
        shaderModules.emplace_back(shaderManager.CreateShaderModule(FilePath(fragmentShaderPath), ShaderType::eFragment));

        return shaderModules;
    }

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

        return RenderPassBuilder(vulkanContext)
            .SetBindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS)
            .SetMultisampling(vulkanContext.GetDevice().GetMaxSampleCount())
            .AddColorAndResolveAttachments(colorAttachmentDescription, resolveAttachmentDescription)
            .AddDepthStencilAttachment(depthStencilAttachmentDescription)
            .Build();
    }
}

RenderSystem::RenderSystem(const Window& window, EventSystem& aEventSystem, const VulkanContext& aVulkanContext)
    : vulkanContext{ aVulkanContext }
    , device{ vulkanContext.GetDevice() }
    , eventSystem{ aEventSystem }
    , uiRenderer{ std::make_unique<UiRenderer>(window, eventSystem, vulkanContext) }
{
    using namespace RenderSystemDetails;
    using namespace VulkanConfig;

    renderPass = CreateRenderPass(vulkanContext);

    CreateAttachmentsAndFramebuffers();

    frames.resize(maxFramesInFlight);
    std::ranges::generate(frames, [&]() { return Frame(&vulkanContext); });

    eventSystem.Subscribe<ES::BeforeSwapchainRecreated>(this, &RenderSystem::OnBeforeSwapchainRecreated);
    eventSystem.Subscribe<ES::SwapchainRecreated>(this, &RenderSystem::OnSwapchainRecreated);
    eventSystem.Subscribe<ES::SceneOpened>(this, &RenderSystem::OnSceneOpen);
    eventSystem.Subscribe<ES::SceneClosed>(this, &RenderSystem::OnSceneClose);
    eventSystem.Subscribe<ES::KeyInput>(this, &RenderSystem::OnKeyInput);
}

RenderSystem::~RenderSystem()
{
    eventSystem.UnsubscribeAll(this);

    device.WaitIdle();
    
    vulkanContext.GetDescriptorSetsManager().ResetDescriptors(DescriptorScope::eGlobal);

    DestroyAttachmentsAndFramebuffers();
}

void RenderSystem::Process(const float deltaSeconds)
{
    if (!scene)
    {
        return;
    }

    const CameraComponent& camera = scene->GetCamera();

    ubo.view = camera.GetViewMatrix();
    ubo.projection = camera.GetProjectionMatrix();
    ubo.viewPos = camera.GetPosition();

    uiRenderer->Process(deltaSeconds);
}

void RenderSystem::Render()
{
    using namespace RenderSystemDetails;
    using namespace VulkanHelpers;

    if (!scene)
    {
        return;
    }
    
    Frame& frame = frames[currentFrame];

    const auto& [waitSemaphores, waitStages, signalSemaphores, fence] = frame.GetSync().AsTuple();

    // Wait for frame to finish execution and signal fence
    vkWaitForFences(device.GetVkDevice(), 1, &fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device.GetVkDevice(), 1, &fence);

    // Acquire next image from the swapchain, frame wait semaphore will be signaled by the presentation engine when it
    // finishes using the image so we can start rendering
    const uint32_t imageIndex = AcquireNextImage(frame);

    // Fill frame data
    frame.GetUniformBuffer().Fill(std::span{ reinterpret_cast<const std::byte*>(&ubo), sizeof(ubo) });

    // Submit scene rendering commands
    const auto renderCommands = [&](VkCommandBuffer buffer) {
        RenderScene(frame, framebuffers[imageIndex]);
        uiRenderer->Render(frame.GetCommandBuffer(), imageIndex);
    };

    SubmitCommandBuffer(frame.GetCommandBuffer(), device.GetQueues().graphics, renderCommands, frame.GetSync());

    // Present will happen when rendering is finished and the frame signal semaphore is signaled
    Present(frame, imageIndex);

    currentFrame = (currentFrame + 1) % VulkanConfig::maxFramesInFlight;
}

uint32_t RenderSystem::AcquireNextImage(const Frame& frame) const
{
    const VkSwapchainKHR swapchain = vulkanContext.GetSwapchain().GetVkSwapchainKHR();

    uint32_t imageIndex;
    const VkResult acquireResult = vkAcquireNextImageKHR(device.GetVkDevice(), swapchain, 
        std::numeric_limits<uint64_t>::max(), frame.GetSync().GetWaitSemaphores()[0], VK_NULL_HANDLE, &imageIndex);
    Assert(acquireResult == VK_SUCCESS || acquireResult == VK_SUBOPTIMAL_KHR);

    return imageIndex;
}

void RenderSystem::RenderScene(const Frame& frame, const VkFramebuffer framebuffer) const
{
    using namespace VulkanHelpers;

    const VkCommandBuffer commandBuffer = frame.GetCommandBuffer();

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass.GetVkRenderPass();
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = vulkanContext.GetSwapchain().GetExtent();

    std::array<VkClearValue, 3> clearValues{};
    clearValues[0].color = { { 0.73f, 0.95f, 1.0f, 1.0f } };
    clearValues[2].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetVkPipeline());

    const VkExtent2D extent = vulkanContext.GetSwapchain().GetExtent();
    VkViewport viewport = GetViewport(static_cast<float>(extent.width), static_cast<float>(extent.height));
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    const VkRect2D scissor = GetScissor(extent);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = { scene->GetVertexBuffer().GetVkBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(commandBuffer, scene->GetIndexBuffer().GetVkBuffer(), 0, VK_INDEX_TYPE_UINT32);

    std::vector<VkDescriptorSet> descriptors = frame.GetDescriptors();
    std::ranges::copy(scene->GetGlobalDescriptors(), std::back_inserter(descriptors));

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetPipelineLayout(),
        0, static_cast<uint32_t>(descriptors.size()), descriptors.data(), 0, nullptr);

    vkCmdDrawIndexedIndirect(commandBuffer, indirectBuffer->GetVkBuffer(), 0, indirectDrawCount,
        sizeof(VkDrawIndexedIndirectCommand));

    vkCmdEndRenderPass(commandBuffer);
}

void RenderSystem::Present(const Frame& frame, const uint32_t imageIndex) const
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    const std::vector<VkSemaphore> signalSemaphores = frame.GetSync().GetSignalSemaphores();
    presentInfo.waitSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
    presentInfo.pWaitSemaphores = signalSemaphores.data();

    VkSwapchainKHR swapchains[] = { vulkanContext.GetSwapchain().GetVkSwapchainKHR() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    const VkResult presentResult = vkQueuePresentKHR(device.GetQueues().present, &presentInfo);
    Assert(presentResult == VK_SUCCESS);
}

void RenderSystem::CreateAttachmentsAndFramebuffers()
{
    using namespace VulkanHelpers;

    const Swapchain& swapchain = vulkanContext.GetSwapchain();

    // Attachments
    colorAttachment = CreateColorAttachment(swapchain.GetExtent(), vulkanContext);
    colorAttachmentView = std::make_unique<ImageView>(*colorAttachment, VK_IMAGE_ASPECT_COLOR_BIT, vulkanContext);

    depthAttachment = CreateDepthAttachment(swapchain.GetExtent(), vulkanContext);
    depthAttachmentView = std::make_unique<ImageView>(*depthAttachment, VK_IMAGE_ASPECT_DEPTH_BIT, vulkanContext);

    // Framebuffers
    framebuffers.reserve(swapchain.GetImageViews().size());

    std::vector<VkImageView> attachments = { colorAttachmentView->GetVkImageView(), VK_NULL_HANDLE,
        depthAttachmentView->GetVkImageView() };

    std::ranges::transform(swapchain.GetImageViews(), std::back_inserter(framebuffers), [&](const ImageView& imageView) {
        attachments[1] = imageView.GetVkImageView();
        return CreateFrameBuffer(renderPass, swapchain.GetExtent(), attachments, vulkanContext);
    });
}

void RenderSystem::DestroyAttachmentsAndFramebuffers()
{
    using namespace VulkanHelpers;

    DestroyFramebuffers(framebuffers, vulkanContext);

    colorAttachmentView.reset();
    colorAttachment.reset();

    depthAttachmentView.reset();
    depthAttachment.reset();
}

// TODO: Do this in a proper place
void RenderSystem::CreateGraphicsPipeline(std::vector<ShaderModule>&& shaderModules)
{
    using namespace VulkanHelpers;

    std::vector descriptorSetLayouts = { frames[0].GetDescriptorSetLayout().GetVkDescriptorSetLayout(),
        Scene::GetGlobalDescriptorSetLayout(vulkanContext).GetVkDescriptorSetLayout() };

    graphicsPipeline = GraphicsPipelineBuilder(vulkanContext)
        .SetDescriptorSetLayouts(std::move(descriptorSetLayouts))
        .AddPushConstantRange( { VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants) })
        .SetShaderModules(std::move(shaderModules))
        .SetVertexData(Vertex::GetBindings(), Vertex::GetAttributes())
        .SetInputTopology(InputTopology::eTriangleList)
        .SetPolygonMode(PolygonMode::eFill)
        .SetCullMode(CullMode::eBack, false)
        .SetMultisampling(vulkanContext.GetDevice().GetMaxSampleCount())
        .SetDepthState(true, true, VK_COMPARE_OP_LESS_OR_EQUAL)
        .SetRenderPass(renderPass)
        .Build();
}

void RenderSystem::TryReloadShaders()
{
    using namespace RenderSystemDetails;

    if (std::vector<ShaderModule> shaderModules = GetShaderModules(vulkanContext.GetShaderManager()); 
        std::ranges::all_of(shaderModules, &ShaderModule::IsValid))
    {
        CreateGraphicsPipeline(std::move(shaderModules));
    }

    uiRenderer->TryReloadShaders();
}

void RenderSystem::OnBeforeSwapchainRecreated(const ES::BeforeSwapchainRecreated& event)
{
    DestroyAttachmentsAndFramebuffers();
}

void RenderSystem::OnSwapchainRecreated(const ES::SwapchainRecreated& event)
{
    CreateAttachmentsAndFramebuffers();
}

void RenderSystem::OnSceneOpen(const ES::SceneOpened& event)
{
    using namespace RenderSystemDetails;
    using namespace VulkanHelpers;

    scene = &event.scene;

    if (!graphicsPipeline.IsValid())
    {
        std::vector<ShaderModule> shaderModules = GetShaderModules(vulkanContext.GetShaderManager());

        const bool allValid = std::ranges::all_of(shaderModules, &ShaderModule::IsValid);
        Assert(allValid);

        CreateGraphicsPipeline(std::move(shaderModules));
    }

    std::tie(indirectBuffer, indirectDrawCount) = CreateIndirectBuffer(*scene, vulkanContext);
}

void RenderSystem::OnSceneClose(const ES::SceneClosed& event)
{        
    scene = nullptr;
}

void RenderSystem::OnKeyInput(const ES::KeyInput& event)
{
    if (event.key == Key::eR && event.action == KeyAction::ePress &&
        HasMod(event.mods, ctrlKeyMod))
    {
        TryReloadShaders();
    }
}
