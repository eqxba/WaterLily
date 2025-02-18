#include "Engine/Render/Resources/DescriptorSets/DescriptorSetLayout.hpp"

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

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
    return VulkanHelpers::GetBinding(*bindings, index);
}