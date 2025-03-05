#pragma once

#include <volk.h>

class DescriptorSetLayout
{
public:
    DescriptorSetLayout() = default;
    DescriptorSetLayout(VkDescriptorSetLayout layout, const std::vector<VkDescriptorSetLayoutBinding>& bindings);

    const VkDescriptorSetLayoutBinding& GetBinding(uint32_t index) const;

    bool IsValid() const
    {
        return layout != VK_NULL_HANDLE;
    }
    
    operator VkDescriptorSetLayout() const
    {
        return layout;
    }

private:
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    const std::vector<VkDescriptorSetLayoutBinding>* bindings = nullptr;
};
