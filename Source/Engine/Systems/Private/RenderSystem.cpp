#include "Engine/Systems/RenderSystem.hpp"

#include "Engine/EventSystem.hpp"
#include "Engine/Render/SceneRenderer.hpp"
#include "Engine/Render/Ui/UiRenderer.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

RenderSystem::RenderSystem(const Window& window, EventSystem& aEventSystem, const VulkanContext& aVulkanContext)
    : vulkanContext{ aVulkanContext }
    , eventSystem{ aEventSystem }
    , sceneRenderer{ std::make_unique<SceneRenderer>(eventSystem, vulkanContext) }
    , uiRenderer{ std::make_unique<UiRenderer>(window, eventSystem, vulkanContext) }
{
    frames.resize(VulkanConfig::maxFramesInFlight);
    std::ranges::generate(frames, [&]() { return Frame(&vulkanContext); });

    eventSystem.Subscribe<ES::KeyInput>(this, &RenderSystem::OnKeyInput);
}

RenderSystem::~RenderSystem()
{
    eventSystem.UnsubscribeAll(this);

    vulkanContext.GetDevice().WaitIdle();
    
    vulkanContext.GetDescriptorSetsManager().ResetDescriptors(DescriptorScope::eGlobal);
}

void RenderSystem::Process(const float deltaSeconds)
{
    sceneRenderer->Process(deltaSeconds);
    uiRenderer->Process(deltaSeconds);
}

void RenderSystem::Render()
{
    using namespace VulkanHelpers;

    const Frame& frame = frames[currentFrame];

    const auto& [waitSemaphores, waitStages, signalSemaphores, fence] = frame.GetSync().AsTuple();

    const Device& device = vulkanContext.GetDevice();

    // Wait for frame to finish execution and signal fence
    vkWaitForFences(device.GetVkDevice(), 1, &fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device.GetVkDevice(), 1, &fence);

    // Acquire next image from the swapchain, frame wait semaphore will be signaled by the presentation engine when it
    // finishes using the image so we can start rendering
    const uint32_t imageIndex = AcquireNextImage(frame);

    // Submit scene rendering commands
    const auto renderCommands = [&](VkCommandBuffer buffer) {
        sceneRenderer->Render(frame.GetCommandBuffer(), currentFrame, imageIndex);
        uiRenderer->Render(frame.GetCommandBuffer(), currentFrame, imageIndex);
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
    const VkResult acquireResult = vkAcquireNextImageKHR(vulkanContext.GetDevice().GetVkDevice(), swapchain,
        std::numeric_limits<uint64_t>::max(), frame.GetSync().GetWaitSemaphores()[0], VK_NULL_HANDLE, &imageIndex);
    Assert(acquireResult == VK_SUCCESS || acquireResult == VK_SUBOPTIMAL_KHR);

    return imageIndex;
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

    const VkResult presentResult = vkQueuePresentKHR(vulkanContext.GetDevice().GetQueues().present, &presentInfo);
    Assert(presentResult == VK_SUCCESS);
}

void RenderSystem::OnKeyInput(const ES::KeyInput& event)
{
    if (event.key == Key::eR && event.action == KeyAction::ePress &&
        HasMod(event.mods, ctrlKeyMod))
    {
        eventSystem.Fire<ES::TryReloadShaders>();
    }
}
