#pragma once

#include <volk.h>

struct BindingReflection
{
    uint32_t index = 0;
    std::string name;
    VkDescriptorType type;
    VkShaderStageFlags shaderStages;
};

struct DescriptorSetReflection
{
    uint32_t index = 0;
    std::vector<BindingReflection> bindings;
};
