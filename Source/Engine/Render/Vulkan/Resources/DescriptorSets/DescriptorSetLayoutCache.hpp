#pragma once

#include <volk.h>

#include "Engine/Render/Vulkan/Resources/DescriptorSets/DescriptorSetLayout.hpp"
#include "Engine/Render/Vulkan/Resources/DescriptorSets/DescriptorSetLayoutBuilder.hpp"

class VulkanContext;

class DescriptorSetLayoutCache
{
public:
    using LayoutHash = size_t;

    explicit DescriptorSetLayoutCache(const VulkanContext& vulkanContext);
    ~DescriptorSetLayoutCache();

    DescriptorSetLayoutCache(const DescriptorSetLayoutCache&) = delete;
    DescriptorSetLayoutCache& operator=(const DescriptorSetLayoutCache&) = delete;

    DescriptorSetLayoutCache(DescriptorSetLayoutCache&&) = delete;
    DescriptorSetLayoutCache& operator=(DescriptorSetLayoutCache&&) = delete;

    DescriptorSetLayout GetLayout(DescriptorSetLayoutBuilder& layoutBuilder);

private:
    const VulkanContext* vulkanContext;

    std::unordered_map<LayoutHash, VkDescriptorSetLayout> layoutCache;
    std::unordered_map<LayoutHash, std::vector<VkDescriptorSetLayoutBinding>> layoutBindings;
};