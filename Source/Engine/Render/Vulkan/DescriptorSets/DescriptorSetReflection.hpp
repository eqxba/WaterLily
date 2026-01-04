#pragma once

#include <volk.h>

struct BindingReflection
{
    uint32_t index = 0;
    std::vector<std::string> names; // Same binging might have multiple names
    VkDescriptorType type;
    VkShaderStageFlags shaderStages;
};

struct DescriptorSetReflection
{
    uint32_t index = 0;
    std::vector<BindingReflection> bindings;
};
