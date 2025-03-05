#include "Engine/Render/Vulkan/VulkanUtils.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/Synchronization/CommandBufferSync.hpp"
#include "Engine/Render/Vulkan/Buffer/Buffer.hpp"
#include "Engine/Render/Vulkan/Image/ImageView.hpp"
#include "Engine/Render/Vulkan/Image/Image.hpp"
#include "Engine/Scene/Scene.hpp"

std::vector<VkCommandBuffer> VulkanUtils::CreateCommandBuffers(VkDevice device, const size_t count, 
    VkCommandPool commandPool)
{
    std::vector<VkCommandBuffer> commandBuffers;
    commandBuffers.resize(count);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    VkResult result = vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data());
    Assert(result == VK_SUCCESS);

    return commandBuffers;
}

void VulkanUtils::SubmitCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, const DeviceCommands& commands,
    const CommandBufferSync& sync)
{
    const auto& [waitSemaphores, waitStages, signalSemaphores, fence] = sync.AsTuple();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    // FYI: if buffer was created with VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT flag vkBeginCommandBuffer
    // implicitly resets the buffer to the initial state
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    commands(commandBuffer);
    VkResult result = vkEndCommandBuffer(commandBuffer);
    Assert(result == VK_SUCCESS);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitStages.data();
    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
    submitInfo.pSignalSemaphores = signalSemaphores.data();

    result = vkQueueSubmit(queue, 1, &submitInfo, fence);
    Assert(result == VK_SUCCESS);
}

VkFence VulkanUtils::CreateFence(VkDevice device, VkFenceCreateFlags flags)
{
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = flags;

    VkFence fence;
    const VkResult result = vkCreateFence(device, &fenceInfo, nullptr, &fence);
    Assert(result == VK_SUCCESS);

    return fence;
}

std::vector<VkFence> VulkanUtils::CreateFences(VkDevice device, VkFenceCreateFlags flags, size_t count)
{
    std::vector<VkFence> fences(count);

    std::ranges::generate(fences, [&]() {
        return CreateFence(device, flags);
    });

    return fences;
}

void VulkanUtils::DestroyFences(VkDevice device, std::vector<VkFence>& fences)
{
    std::ranges::for_each(fences, [&](VkFence fence) {
        vkDestroyFence(device, fence, nullptr);
    });

    fences.clear();
}

VkSemaphore VulkanUtils::CreateSemaphore(VkDevice device)
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkSemaphore semaphore;
    const VkResult result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore);
    Assert(result == VK_SUCCESS);
    
    return semaphore;
}

std::vector<VkSemaphore> VulkanUtils::CreateSemaphores(VkDevice device, size_t count)
{
    std::vector<VkSemaphore> semaphores(count);
    
    std::ranges::generate(semaphores, [&]() {
        return CreateSemaphore(device);
    });

    return semaphores;
}

void VulkanUtils::DestroySemaphores(VkDevice device, std::vector<VkSemaphore>& semaphores)
{
    std::ranges::for_each(semaphores, [&](VkSemaphore semaphore) {
        if (semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(device, semaphore, nullptr);
        }        
    });

    semaphores.clear();
}

VkFramebuffer VulkanUtils::CreateFrameBuffer(const RenderPass& renderPass, const VkExtent2D extent,
    const std::vector<VkImageView>& attachments, const VulkanContext& vulkanContext)
{
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = extent.width;
    framebufferInfo.height = extent.height;
    framebufferInfo.layers = 1;

    VkFramebuffer framebuffer;
    const VkResult result = vkCreateFramebuffer(vulkanContext.GetDevice(), &framebufferInfo, nullptr, &framebuffer);
    Assert(result == VK_SUCCESS);

    return framebuffer;
}

void VulkanUtils::DestroyFramebuffers(std::vector<VkFramebuffer>& framebuffers, const VulkanContext& vulkanContext)
{
    std::ranges::for_each(framebuffers, [&](VkFramebuffer framebuffer) {
        vkDestroyFramebuffer(vulkanContext.GetDevice(), framebuffer, nullptr);
    });

    framebuffers.clear();
}

std::tuple<std::unique_ptr<Buffer>, uint32_t> VulkanUtils::CreateIndirectBuffer(const Scene& scene,
    const VulkanContext& vulkanContext)
{
    std::vector<VkDrawIndexedIndirectCommand> indirectCommands;

    const auto createDrawCommand = [&](const Primitive& primitive) {
        VkDrawIndexedIndirectCommand indirectCommand{};
        indirectCommand.instanceCount = 1;
        indirectCommand.firstIndex = primitive.firstIndex;
        indirectCommand.indexCount = primitive.indexCount;
        indirectCommand.firstInstance = primitive.nodeId; // Using this to access transforms SSBO in shaders

        return indirectCommand;
    };

    std::ranges::for_each(scene.GetNodes(), [&](const SceneNode* node) {

        if (!node->visible)
        {
            return;
        }

        std::ranges::transform(node->mesh.primitives, std::back_inserter(indirectCommands), createDrawCommand);
    });

    std::span indirectCommandsSpan(std::as_const(indirectCommands));

    BufferDescription indirectBufferDescription{ .size = static_cast<VkDeviceSize>(indirectCommandsSpan.size_bytes()),
        .usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

    return { std::make_unique<Buffer>(indirectBufferDescription, true, indirectCommandsSpan, vulkanContext),
        static_cast<uint32_t>(indirectCommands.size()) };
}

VkViewport VulkanUtils::GetViewport(const float width, const float height)
{
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    return viewport;
}

VkRect2D VulkanUtils::GetScissor(const VkExtent2D extent)
{
    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = extent;

    return scissor;
}

VkPipelineLayout VulkanUtils::CreatePipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
    const std::vector<VkPushConstantRange>& pushConstantRanges, const VkDevice device)
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

    VkPipelineLayout pipelineLayout;

    const VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
    Assert(result == VK_SUCCESS);

    return pipelineLayout;
}

const VkDescriptorSetLayoutBinding& VulkanUtils::GetBinding(const std::vector<VkDescriptorSetLayoutBinding>& bindings, 
    const uint32_t index)
{
    const auto it = std::ranges::find_if(bindings, [&](const auto& binding) { return binding.binding == index; });

    Assert(it != bindings.end());

    return *it;
}
