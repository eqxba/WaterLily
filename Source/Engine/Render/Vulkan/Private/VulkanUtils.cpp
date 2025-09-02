#include "Engine/Render/Vulkan/VulkanUtils.hpp"

#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Buffer/BufferUtils.hpp"
#include "Engine/Render/Vulkan/Synchronization/CommandBufferSync.hpp"
#include "Engine/Render/Vulkan/Synchronization/SynchronizationUtils.hpp"

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
    const std::span<const VkImageView> attachments, const VulkanContext& vulkanContext)
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

VkPipelineLayout VulkanUtils::CreatePipelineLayout(const std::vector<DescriptorSetLayout>& descriptorSetLayouts,
    const std::vector<VkPushConstantRange>& pushConstantRanges, VkDevice device)
{
    std::vector<VkDescriptorSetLayout> convertedLayouts;
    convertedLayouts.reserve(descriptorSetLayouts.size());
    
    for (const DescriptorSetLayout& layout : descriptorSetLayouts)
    {
        convertedLayouts.push_back(layout);
    }
    
    return CreatePipelineLayout(convertedLayouts, pushConstantRanges, device);
}

const VkDescriptorSetLayoutBinding& VulkanUtils::GetBinding(const std::vector<VkDescriptorSetLayoutBinding>& bindings, 
    const uint32_t index)
{
    const auto it = std::ranges::find_if(bindings, [&](const auto& binding) { return binding.binding == index; });

    Assert(it != bindings.end());

    return *it;
}

std::vector<DescriptorSetLayout> VulkanUtils::CreateDescriptorSetLayouts(const std::vector<DescriptorSetReflection>& reflections,
    const VulkanContext& vulkanContext)
{
    std::vector<DescriptorSetLayout> layouts;
    layouts.reserve(reflections.size());
    
    for (const DescriptorSetReflection& reflection : reflections)
    {
        std::unordered_set<uint32_t> boundIndexes;
        DescriptorSetLayoutBuilder builder = vulkanContext.GetDescriptorSetsManager().GetDescriptorSetLayoutBuilder();
        
        for (const BindingReflection& binding : reflection.bindings)
        {
            if (!boundIndexes.contains(binding.index))
            {
                boundIndexes.insert(binding.index);
                builder.AddBinding(binding.index, binding.type, binding.shaderStages);
            }
        }
        
        layouts.push_back(builder.Build());
    }
    
    return layouts;
}

VkRenderPassBeginInfo VulkanUtils::GetRenderPassBeginInfo(const VkRenderPass renderPass, const VkFramebuffer framebuffer,
    const VkRect2D renderArea, const std::span<const VkClearValue> clearValues)
{
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = renderArea.offset;
    renderPassInfo.renderArea.extent = renderArea.extent;
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    
    return renderPassInfo;
}

void VulkanUtils::SetDefaultViewportAndScissor(const VkCommandBuffer commandBuffer, const Swapchain& swapchain)
{
    const VkExtent2D extent = swapchain.GetExtent();
    
    VkViewport viewport = GetViewport(static_cast<float>(extent.width), static_cast<float>(extent.height));
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    const VkRect2D scissor = GetScissor(extent);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}
