#include "Engine/Render/Vulkan/Resources/DescriptorSets/DescriptorSetLayout.hpp"

#include "Engine/Render/Vulkan/VulkanUtils.hpp"

DescriptorSetLayout::DescriptorSetLayout(const VkDescriptorSetLayout aLayout, 
    const std::vector<VkDescriptorSetLayoutBinding>& aBindings)
    : layout{ aLayout }
    , bindings{ &aBindings }
{
    Assert(IsValid());
    Assert(!bindings->empty());
}

const VkDescriptorSetLayoutBinding& DescriptorSetLayout::GetBinding(const uint32_t index) const
{
    Assert(IsValid());
    return VulkanUtils::GetBinding(*bindings, index);
}