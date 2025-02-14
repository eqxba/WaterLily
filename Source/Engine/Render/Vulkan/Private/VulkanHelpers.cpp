#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/CommandBufferSync.hpp"
#include "Engine/Render/Vulkan/Resources/Buffer.hpp"
#include "Engine/Scene/Scene.hpp"

std::vector<VkCommandBuffer> VulkanHelpers::CreateCommandBuffers(VkDevice device, const size_t count, 
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

void VulkanHelpers::SubmitCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, DeviceCommands commands, 
    const CommandBufferSync& sync)
{
    const auto& [waitSemaphores, waitStages, signalSemaphores, fence] = sync.AsTuple();

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

VkSemaphore VulkanHelpers::CreateSemaphore(VkDevice device)
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkSemaphore semaphore;
    const VkResult result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore);
    Assert(result == VK_SUCCESS);
    
    return semaphore;
}

std::vector<VkSemaphore> VulkanHelpers::CreateSemaphores(VkDevice device, size_t count)
{
    std::vector<VkSemaphore> semaphores(count);
    
    std::ranges::generate(semaphores, [&]() {
        return CreateSemaphore(device);
    });

    return semaphores;
}

void VulkanHelpers::DestroyFences(VkDevice device, std::vector<VkFence>& fences)
{
    std::ranges::for_each(fences, [&](VkFence fence) {
        vkDestroyFence(device, fence, nullptr);
    });   

    fences.clear();
}

void VulkanHelpers::DestroySemaphores(VkDevice device, std::vector<VkSemaphore>& semaphores)
{
    std::ranges::for_each(semaphores, [&](VkSemaphore semaphore) {
        if (semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(device, semaphore, nullptr);
        }        
    });

    semaphores.clear();
}

VkSampler VulkanHelpers::CreateSampler(VkDevice device, VkPhysicalDeviceProperties properties, uint32_t mipLevelsCount)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(mipLevelsCount);

    VkSampler sampler;
    VkResult result = vkCreateSampler(device, &samplerInfo, nullptr, &sampler);
    Assert(result == VK_SUCCESS);

    return sampler;
}

void VulkanHelpers::DestroySampler(VkDevice device, VkSampler sampler)
{
    if (sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(device, sampler, nullptr);
    }
}

std::vector<VkDescriptorSet> VulkanHelpers::CreateDescriptorSets(VkDescriptorPool descriptorPool,
    VkDescriptorSetLayout layout, size_t count, VkDevice device)
{
    std::vector<VkDescriptorSet> descriptorSets(count);

    std::vector<VkDescriptorSetLayout> layouts(count, layout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(count);
    allocInfo.pSetLayouts = layouts.data();

    const VkResult result = vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data());
    Assert(result == VK_SUCCESS);

    return descriptorSets;
}

void VulkanHelpers::PopulateDescriptorSet(const VkDescriptorSet set, const Buffer& uniformBuffer, const Scene& scene,
    VkDevice device)
{
    std::vector<VkWriteDescriptorSet> descriptorWrites;
    
    // Sampler

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = scene.GetImageView().GetVkImageView();
    imageInfo.sampler = scene.GetSampler();
    
    VkWriteDescriptorSet samplerWrite{};
    samplerWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    samplerWrite.dstSet = set;
    samplerWrite.dstBinding = 1;
    samplerWrite.dstArrayElement = 0;
    samplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerWrite.descriptorCount = 1;
    samplerWrite.pImageInfo = &imageInfo;
    
    descriptorWrites.push_back(samplerWrite);

    // Uniform buffer
    
    VkDescriptorBufferInfo uniformBufferInfo{};
    uniformBufferInfo.buffer = uniformBuffer.GetVkBuffer();
    uniformBufferInfo.offset = 0;
    uniformBufferInfo.range = uniformBuffer.GetDescription().size;
    
    VkWriteDescriptorSet uniformBufferWrite{};
    uniformBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    uniformBufferWrite.dstSet = set;
    uniformBufferWrite.dstBinding = 0;
    uniformBufferWrite.dstArrayElement = 0;
    uniformBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformBufferWrite.descriptorCount = 1;
    uniformBufferWrite.pBufferInfo = &uniformBufferInfo;
    
    descriptorWrites.push_back(uniformBufferWrite);

    // Transforms SSBO

    const Buffer& transformsSSBO = scene.GetTransformsBuffer();
    
    VkDescriptorBufferInfo transformsSSBOBufferInfo;
    transformsSSBOBufferInfo.buffer = transformsSSBO.GetVkBuffer();
    transformsSSBOBufferInfo.offset = 0;
    transformsSSBOBufferInfo.range = transformsSSBO.GetDescription().size;
    
    VkWriteDescriptorSet transformsSSBOWrite{};
    transformsSSBOWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    transformsSSBOWrite.dstSet = set;
    transformsSSBOWrite.dstBinding = 2;
    transformsSSBOWrite.dstArrayElement = 0;
    transformsSSBOWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    transformsSSBOWrite.descriptorCount = 1;
    transformsSSBOWrite.pBufferInfo = &transformsSSBOBufferInfo;

    descriptorWrites.push_back(transformsSSBOWrite);

    // Do the update

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}
