#include "Engine/Systems/RenderSystem.hpp"

#include "Engine/EventSystem.hpp"
#include "Engine/Render/SceneRenderer.hpp"
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
        std::vector<VkSemaphore> signalSemaphores = { CreateSemaphore(device) };
        VkFence fence = CreateFence(device, VK_FENCE_CREATE_SIGNALED_BIT);
        
        return { waitSemaphores, waitStages, signalSemaphores, fence, device };
    }

    static VkCommandBuffer CreateCommandBuffer(const VulkanContext& vulkanContext)
    {
        const Device& device = vulkanContext.GetDevice();
        const VkCommandPool longLivedPool = device.GetCommandPool(CommandBufferType::eLongLived);
        
        return VulkanUtils::CreateCommandBuffers(device, 1, longLivedPool)[0];
    }

    static VkQueryPool CreateQueryPool(const VulkanContext& vulkanContext)
    {
        VkQueryPoolCreateInfo queryPoolInfo = { .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
        queryPoolInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
        queryPoolInfo.pipelineStatistics = VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT;
        queryPoolInfo.queryCount = VulkanConfig::maxFramesInFlight;

        VkQueryPool queryPool;
        const VkResult result = vkCreateQueryPool(vulkanContext.GetDevice(), &queryPoolInfo, nullptr, &queryPool);
        Assert(result == VK_SUCCESS);

        vulkanContext.GetDevice().ExecuteOneTimeCommandBuffer([&](const VkCommandBuffer cmd) {
            vkCmdResetQueryPool(cmd, queryPool, 0, VulkanConfig::maxFramesInFlight);

            for (uint32_t i = 0; i < VulkanConfig::maxFramesInFlight; ++i)
            {
                // Initialize to have zero values for the first maxFramesInFlight frames
                vkCmdBeginQuery(cmd, queryPool, i, 0);
                vkCmdEndQuery(cmd, queryPool, i);
            }
        });

        return queryPool;
    }
}

RenderSystem::RenderSystem(const Window& window, EventSystem& aEventSystem, const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
    , renderOptions{ &RenderOptions::Get() }
    , eventSystem{ aEventSystem }
    , sceneRenderer{ std::make_unique<SceneRenderer>(eventSystem, aVulkanContext) }
    , computeRenderer{ std::make_unique<ComputeRenderer>(eventSystem, aVulkanContext) }
    , uiRenderer{ std::make_unique<UiRenderer>(window, eventSystem, aVulkanContext) }
{
    using namespace RenderSystemDetails;
    
    const auto createFrame = [&](const uint32_t index) {
        return Frame(index, 0, CreateCommandBuffer(*vulkanContext), CreateFrameSync(*vulkanContext));
    };
    
    constexpr auto frameIndices = std::views::iota(static_cast<uint32_t>(0), VulkanConfig::maxFramesInFlight);
    std::ranges::transform(frameIndices, std::back_inserter(frames), createFrame);
    
    SetRenderer(renderOptions->GetRendererType());

    if (vulkanContext->GetDevice().GetProperties().pipelineStatisticsQuerySupported)
    {
        queryPool = CreateQueryPool(*vulkanContext);
    }

    eventSystem.Subscribe<ES::KeyInput>(this, &RenderSystem::OnKeyInput);
    eventSystem.Subscribe<RenderOptions::RendererTypeChanged>(this, &RenderSystem::OnRendererTypeChanged);
}

RenderSystem::~RenderSystem()
{
    eventSystem.UnsubscribeAll(this);

    vulkanContext->GetDevice().WaitIdle();
    
    vulkanContext->GetDescriptorSetsManager().ResetDescriptors(DescriptorScope::eGlobal);

    if (vulkanContext->GetDevice().GetProperties().pipelineStatisticsQuerySupported)
    {
        vkDestroyQueryPool(vulkanContext->GetDevice(), queryPool, nullptr);
    }
}

void RenderSystem::Process(const float deltaSeconds)
{
    renderer->Process(frames[currentFrame], deltaSeconds);
    uiRenderer->Process(frames[currentFrame], deltaSeconds);
}

void RenderSystem::Render()
{
    using namespace VulkanUtils;

    Frame& frame = frames[currentFrame];

    const auto& [waitSemaphores, waitStages, signalSemaphores, fence] = frame.sync.AsTuple();

    const Device& device = vulkanContext->GetDevice();

    // Wait for frame to finish execution and signal fence
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &fence);
    
    const bool useQuery = vulkanContext->GetDevice().GetProperties().pipelineStatisticsQuerySupported;

    if (useQuery)
    {
        // Gather gpu frame data
        vkGetQueryPoolResults(device, queryPool, currentFrame, 1, sizeof(frame.stats), &frame.stats,
            sizeof(frame.stats), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    }

    // Acquire next image from the swapchain, frame wait semaphore will be signaled by the presentation engine when it
    // finishes using the image so we can start rendering
    frame.swapchainImageIndex = AcquireNextSwapchainImage(waitSemaphores[0]);

    // Submit scene rendering commands
    const auto renderCommands = [&](VkCommandBuffer commandBuffer) {
        
        SetDefaultViewportAndScissor(commandBuffer, vulkanContext->GetSwapchain());
        
        if (useQuery)
        {
            vkCmdResetQueryPool(commandBuffer, queryPool, currentFrame, 1);
            vkCmdBeginQuery(commandBuffer, queryPool, currentFrame, 0);
        }
        
        renderer->Render(frame);
        
        if (useQuery)
        {
            vkCmdEndQuery(commandBuffer, queryPool, currentFrame);
        }
        
        uiRenderer->Render(frame);
    };

    SubmitCommandBuffer(frame.commandBuffer, device.GetQueues().graphicsAndCompute, renderCommands, frame.sync);

    // Present will happen when rendering is finished and the frame signal semaphores are signaled
    Present(signalSemaphores, frame.swapchainImageIndex);

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
    renderer = rendererType == RendererType::eScene ? sceneRenderer.get() : computeRenderer.get();
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
