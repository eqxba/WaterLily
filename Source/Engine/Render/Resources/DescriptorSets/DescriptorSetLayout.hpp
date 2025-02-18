#pragma once

#include <volk.h>

class DescriptorSetLayout
{
public:
    DescriptorSetLayout() = default;
    DescriptorSetLayout(VkDescriptorSetLayout layout, const std::vector<VkDescriptorSetLayoutBinding>& bindings);
    ~DescriptorSetLayout() = default;

    DescriptorSetLayout(const DescriptorSetLayout&) = default;
    DescriptorSetLayout& operator=(const DescriptorSetLayout&) = default;

    DescriptorSetLayout(DescriptorSetLayout&& other) = default;
    DescriptorSetLayout& operator=(DescriptorSetLayout&& other) = default;

    const VkDescriptorSetLayoutBinding& GetBinding(uint32_t index) const;

    VkDescriptorSetLayout GetVkDescriptorSetLayout() const
    {
        return layout;
    }

    bool IsValid() const
    {
        return layout != VK_NULL_HANDLE;
    }

private:
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    const std::vector<VkDescriptorSetLayoutBinding>* bindings = nullptr;
};