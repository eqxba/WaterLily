#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

void VulkanHelpers::SubmitCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, DeviceCommands commands, 
    const CommandBufferSync& sync)
{
    const auto& [waitSemaphores, waitStages, signalSemaphores, fence] = sync;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    // FYI: vkBeginCommandBuffer if buffer was created with VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT flag
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

VkFence VulkanHelpers::CreateFence(VkDevice device, VkFenceCreateFlags flags)
{
    VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = flags;

    VkFence fence;
	const VkResult result = vkCreateFence(device, &fenceInfo, nullptr, &fence);
	Assert(result == VK_SUCCESS);

    return fence;
}

std::vector<VkFence> VulkanHelpers::CreateFences(VkDevice device, VkFenceCreateFlags flags, size_t count)
{
    std::vector<VkFence> fences(count);

    std::ranges::generate(fences, [&]() {
        return CreateFence(device, flags);
    });

	return fences;
}

std::vector<VkSemaphore> VulkanHelpers::CreateSemaphores(VkDevice device, size_t count)
{
    std::vector<VkSemaphore> semaphores(count);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    const auto createSemaphore = [&]() {
        VkSemaphore semaphore;
        const VkResult result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore);
        Assert(result == VK_SUCCESS);
        return semaphore;
    };

    std::ranges::generate(semaphores, createSemaphore);

    return semaphores;
}

void VulkanHelpers::DestroyFences(VkDevice device, std::vector<VkFence>& fences)
{
    std::ranges::for_each(fences, [&](VkFence fence) {
        vkDestroyFence(device, fence, nullptr);
    });   
}

void VulkanHelpers::DestroySemaphores(VkDevice device, std::vector<VkSemaphore>& semaphores)
{
    std::ranges::for_each(semaphores, [&](VkSemaphore semaphore) {
        vkDestroySemaphore(device, semaphore, nullptr);
    });
}

void VulkanHelpers::DestroyCommandBufferSync(VkDevice device, CommandBufferSync& sync)
{
    DestroySemaphores(device, sync.waitSemaphores);
    DestroySemaphores(device, sync.signalSemaphores);
    vkDestroyFence(device, sync.fence, nullptr);
}
