#include "Engine/Render/Vulkan/Frame.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

namespace FrameDetails
{
    static CommandBufferSync CreateSync(const VkDevice device)
    {
        using namespace VulkanHelpers;
        
        std::vector<VkSemaphore> waitSemaphores = { CreateSemaphore(device) };
        std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        std::vector<VkSemaphore> signalSemaphores = { CreateSemaphore(device) };
        VkFence fence = CreateFence(device, VK_FENCE_CREATE_SIGNALED_BIT);
        
        return { waitSemaphores, waitStages, signalSemaphores, fence, device };
    }
}

Frame::Frame(const VulkanContext* aVulkanContext)
    : vulkanContext{aVulkanContext}
{
    using namespace VulkanHelpers;
    
    const Device& device = vulkanContext->GetDevice();
    const VkCommandPool longLivedPool = device.GetCommandPool(CommandBufferType::eLongLived);
    
    commandBuffer = CreateCommandBuffers(device.GetVkDevice(), 1, longLivedPool)[0];
    sync = FrameDetails::CreateSync(device.GetVkDevice());
}

Frame::~Frame() = default;

Frame::Frame(Frame&& other) noexcept
    : vulkanContext{ other.vulkanContext }
    , commandBuffer{ other.commandBuffer }
    , sync{ std::move(other.sync) }
{
    other.vulkanContext = nullptr;
    other.commandBuffer = VK_NULL_HANDLE;
    other.sync = {};
}

Frame& Frame::operator=(Frame&& other) noexcept
{
    if (this != &other)
    {
        std::swap(vulkanContext, other.vulkanContext);
        std::swap(commandBuffer, other.commandBuffer);
        std::swap(sync, other.sync);
    }
    return *this;
}
