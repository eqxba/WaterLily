#include "Engine/Systems/RenderSystem.hpp"

#include "Engine/EventSystem.hpp"
#include "Engine/Render/ForwardRenderer.hpp"
#include "Engine/Render/Ui/UiRenderer.hpp"
#include "Engine/Render/ComputeRenderer.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanUtils.hpp"

namespace RenderSystemDetails
{
    static CommandBufferSync CreateFrameSync(const VulkanContext& vulkanContext)
    {
        using namespace VulkanUtils;
        
        const VkDevice device = vulkanContext.GetDevice();
        
        std::vector<VkSemaphore> waitSemaphores = { CreateSemaphore(device) };
        std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        VkFence fence = CreateFence(device, VK_FENCE_CREATE_SIGNALED_BIT);
        
        return { waitSemaphores, waitStages, {}, fence, device };
    }

    static VkCommandBuffer CreateCommandBuffer(const VulkanContext& vulkanContext)
    {
        const Device& device = vulkanContext.GetDevice();
        const VkCommandPool longLivedPool = device.GetCommandPool(CommandBufferType::eLongLived);
        
        return VulkanUtils::CreateCommandBuffers(device, 1, longLivedPool)[0];
    }
}

RenderSystem::RenderSystem(const Window& window, EventSystem& aEventSystem, const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
    , renderOptions{ &RenderOptions::Get() }
    , eventSystem{ aEventSystem }
    , forwardRenderer{ std::make_unique<ForwardRenderer>(eventSystem, aVulkanContext) }
    , computeRenderer{ std::make_unique<ComputeRenderer>(eventSystem, aVulkanContext) }
    , uiRenderer{ std::make_unique<UiRenderer>(window, eventSystem, aVulkanContext) }
{
    using namespace RenderSystemDetails;
    
    const auto createFrame = [&](const uint32_t index) {
        return Frame(index, 0, CreateCommandBuffer(*vulkanContext), CreateFrameSync(*vulkanContext), StatsUtils::CreateFrameQueryPools(*vulkanContext));
    };
    
    constexpr auto frameIndices = std::views::iota(static_cast<uint32_t>(0), VulkanConfig::maxFramesInFlight);
    std::ranges::transform(frameIndices, std::back_inserter(frames), createFrame);

    // We must have a separate "ready to present / finished rendering" semaphore per swapchain image, not per frame in flight
    std::ranges::transform(vulkanContext->GetSwapchain().GetRenderTargets(), std::back_inserter(readyToPresentSemaphores), 
        [&](const auto& _) { return VulkanUtils::CreateSemaphore(vulkanContext->GetDevice()); });

    SetRenderer(renderOptions->GetRendererType());

    eventSystem.Subscribe<ES::KeyInput>(this, &RenderSystem::OnKeyInput);
    eventSystem.Subscribe<RenderOptions::RendererTypeChanged>(this, &RenderSystem::OnRendererTypeChanged);
}

RenderSystem::~RenderSystem()
{
    eventSystem.UnsubscribeAll(this);

    vulkanContext->GetDevice().WaitIdle();

    VulkanUtils::DestroySemaphores(vulkanContext->GetDevice(), readyToPresentSemaphores);
    
    vulkanContext->GetDescriptorSetsManager().ResetDescriptors(DescriptorScope::eGlobal);
    
    std::ranges::for_each(frames, [&](Frame& frame) { StatsUtils::DestroyFrameQueryPools(frame.queryPools, *vulkanContext); });
}

void RenderSystem::Process(const float deltaSeconds)
{
    renderer->Process(frames[currentFrame], deltaSeconds);
    uiRenderer->Process(frames[currentFrame], deltaSeconds);
}

void RenderSystem::Render()
{
    using namespace VulkanUtils;
    using namespace RenderSystemDetails;

    Frame& frame = frames[currentFrame];

    auto [waitSemaphores, waitStages, signalSemaphores, fence] = frame.sync.AsTuple();

    const Device& device = vulkanContext->GetDevice();

    // Wait for frame to finish execution and signal fence
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &fence);
    
    StatsUtils::GatherFrameStats(device, frame.queryPools, frame.renderStats);

    // Acquire next image from the swapchain, frame wait semaphore will be signaled by the presentation engine when it
    // finishes using the image so we can start rendering
    frame.swapchainImageIndex = AcquireNextSwapchainImage(waitSemaphores[0]);

    // Add corresponding to acquired swapchain image signal ("ready to present / finished rendering") semaphore to frame signal semaphores
    signalSemaphores.push_back(readyToPresentSemaphores[frame.swapchainImageIndex]);

    // Submit scene rendering commands
    const auto renderCommands = [&](VkCommandBuffer commandBuffer) {
        
        SetDefaultViewportAndScissor(commandBuffer, vulkanContext->GetSwapchain());
        
        StatsUtils::ResetFrameQueryPools(commandBuffer, frame.queryPools);
        
        StatsUtils::WriteTimestamp(commandBuffer, frame.queryPools.timestamps, GpuTimestamp::eFrameBegin);
        
        if (frame.queryPools.pipelineStats != VK_NULL_HANDLE)
        {
            vkCmdBeginQuery(commandBuffer, frame.queryPools.pipelineStats, 0, 0);
        }
        
        renderer->Render(frame);
        
        if (frame.queryPools.pipelineStats != VK_NULL_HANDLE)
        {
            vkCmdEndQuery(commandBuffer, frame.queryPools.pipelineStats, 0);
        }
        
        uiRenderer->Render(frame);
        
        StatsUtils::WriteTimestamp(commandBuffer, frame.queryPools.timestamps, GpuTimestamp::eFrameEnd);
    };

    SubmitCommandBuffer(frame.commandBuffer, device.GetQueues().graphicsAndCompute, renderCommands, frame.sync);

    // Present will happen when rendering is finished and the frame signal semaphores are signaled
    // along with corresponding to acquired swapchain image readyToPresentSemaphore
    Present(signalSemaphores, frame.swapchainImageIndex);

    // Remove added above readyToPresentSemaphore, as we can get another swapchain image next frame
    signalSemaphores.pop_back();

    currentFrame = (currentFrame + 1) % VulkanConfig::maxFramesInFlight;
}

uint32_t RenderSystem::AcquireNextSwapchainImage(const VkSemaphore signalSemaphore) const
{
    uint32_t imageIndex;
    const VkResult acquireResult = vkAcquireNextImageKHR(vulkanContext->GetDevice(), vulkanContext->GetSwapchain(),
        std::numeric_limits<uint64_t>::max(), signalSemaphore, VK_NULL_HANDLE, &imageIndex);
    Assert(acquireResult == VK_SUCCESS || acquireResult == VK_SUBOPTIMAL_KHR);

    return imageIndex;
}

void RenderSystem::Present(const std::vector<VkSemaphore>& waitSemaphores, const uint32_t imageIndex) const
{
    VkSwapchainKHR swapchains[] = { vulkanContext->GetSwapchain() };
    
    VkPresentInfoKHR presentInfo = { .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    presentInfo.pWaitSemaphores = waitSemaphores.data();
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    const VkResult presentResult = vkQueuePresentKHR(vulkanContext->GetDevice().GetQueues().present, &presentInfo);
    Assert(presentResult == VK_SUCCESS);
}

void RenderSystem::SetRenderer(RendererType aRendererType)
{
    rendererType = aRendererType;
    renderer = rendererType == RendererType::eForward ? forwardRenderer.get() : computeRenderer.get();
}

void RenderSystem::OnKeyInput(const ES::KeyInput& event)
{
    if (event.key == Key::eR && event.action == KeyAction::ePress &&
        HasMod(event.mods, ctrlKeyMod))
    {
        eventSystem.Fire<ES::TryReloadShaders>();
    }
}

void RenderSystem::OnRendererTypeChanged()
{
    SetRenderer(renderOptions->GetRendererType());
}
