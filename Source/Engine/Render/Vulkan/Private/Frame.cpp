#include "Engine/Render/Vulkan/Frame.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Shaders/Common.h"

namespace FrameDetails
{
    static CommandBufferSync CreateSync(const VkDevice device)
    {
        using namespace VulkanHelpers;
        
        std::vector<VkSemaphore> waitSemaphores = { CreateSemaphore(device) };
        std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        std::vector<VkSemaphore> signalSemaphores = { CreateSemaphore(device) };
        VkFence fence = CreateFence(device, VK_FENCE_CREATE_SIGNALED_BIT);
        
        return CommandBufferSync(waitSemaphores, waitStages, signalSemaphores, fence, device);
    }

    static Buffer CreateUniformBuffer(const VulkanContext& vulkanContext)
    {
        BufferDescription bufferDescription{ static_cast<VkDeviceSize>(sizeof(gpu::UniformBufferObject)),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

        auto result = Buffer(bufferDescription, true, &vulkanContext);
        std::ignore = result.GetStagingBuffer()->MapMemory(true);

        return result;
    }
}

Frame::Frame(const VulkanContext* aVulkanContext)
    : vulkanContext{aVulkanContext}
{
    using namespace FrameDetails;
    using namespace VulkanHelpers;
    
    const Device& device = vulkanContext->GetDevice();
    const VkCommandPool longLivedPool = device.GetCommandPool(CommandBufferType::eLongLived);
    
    commandBuffer = CreateCommandBuffers(device.GetVkDevice(), 1, longLivedPool)[0];
    sync = CreateSync(device.GetVkDevice());
    
    uniformBuffer = CreateUniformBuffer(*vulkanContext);

    const auto [descriptor, layout] = vulkanContext->GetDescriptorSetsManager().GetDescriptorSetBuilder()
        .Bind(0, uniformBuffer, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .Build();

    descriptors.push_back(descriptor);
    descriptorSetLayout = layout;
}

Frame::~Frame() = default;

Frame::Frame(Frame&& other) noexcept
    : vulkanContext{ other.vulkanContext }
    , commandBuffer{ other.commandBuffer }
    , sync{ std::move(other.sync) }
    , uniformBuffer{ std::move(other.uniformBuffer) }
    , descriptors{ std::move(other.descriptors) }
    , descriptorSetLayout{ other.descriptorSetLayout }
{
    other.vulkanContext = nullptr;
    other.commandBuffer = VK_NULL_HANDLE;
    other.sync = {};
    other.uniformBuffer = {};
    other.descriptorSetLayout = {};
}

Frame& Frame::operator=(Frame&& other) noexcept
{
    if (this != &other)
    {
        vulkanContext = other.vulkanContext;
        commandBuffer = other.commandBuffer;
        sync = std::move(other.sync);
        uniformBuffer = std::move(other.uniformBuffer);
        descriptors = std::move(other.descriptors);
        descriptorSetLayout = other.descriptorSetLayout;
        
        other.vulkanContext = nullptr;
        other.commandBuffer = VK_NULL_HANDLE;
        other.sync = {};
        other.uniformBuffer = {};
        other.descriptorSetLayout = {};
    }
    return *this;
}
