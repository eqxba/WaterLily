#include "Engine/Render/Resources/DescriptorLayoutCache.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

DescriptorLayoutCache::DescriptorLayoutCache(const VulkanContext& aVulkanContext) : vulkanContext{&aVulkanContext} {}

DescriptorLayoutCache::~DescriptorLayoutCache()
{
    std::ranges::for_each(cache, [&](const auto& entry) {
        const auto& [info, layout] = entry;
        vkDestroyDescriptorSetLayout(vulkanContext->GetDevice().GetVkDevice(), layout, nullptr);
    });
}

VkDescriptorSetLayout DescriptorLayoutCache::CreateLayout(const DescriptorLayout& layout)
{
    if (const auto it = cache.find(layout); it != cache.end())
    {
        return it->second;
    }

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount = static_cast<uint32_t>(layout.GetBindings().size());
	layoutCreateInfo.pBindings = layout.GetBindings().data();

	VkDescriptorSetLayout descriptorSetLayout;

	const VkResult result = vkCreateDescriptorSetLayout(vulkanContext->GetDevice().GetVkDevice(), &layoutCreateInfo, 
		nullptr, &descriptorSetLayout);
	Assert(result == VK_SUCCESS);

	cache[layout] = descriptorSetLayout;

	return descriptorSetLayout;
}