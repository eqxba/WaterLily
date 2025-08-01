#pragma once

#include <volk.h>

#include "Engine/Render/Vulkan/DescriptorSets/DescriptorSetLayout.hpp"

class DescriptorSetLayoutCache;

class DescriptorSetLayoutBuilder
{
public:
    explicit DescriptorSetLayoutBuilder(DescriptorSetLayoutCache& cache);

    DescriptorSetLayout Build();

    DescriptorSetLayoutBuilder& AddBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags shaderStages);

    const VkDescriptorSetLayoutBinding& GetBinding(uint32_t index) const;

    size_t GetHash();

    std::vector<VkDescriptorSetLayoutBinding> MoveBindings();

private:
    bool invalid = false;

    DescriptorSetLayoutCache* cache;

    std::vector<VkDescriptorSetLayoutBinding> bindings;
};
