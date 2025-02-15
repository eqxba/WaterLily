#pragma once

#include <volk.h>

#include "Engine/Render/Resources/Descriptor.hpp"

class VulkanContext;

class DescriptorLayoutCache
{
public:
	explicit DescriptorLayoutCache(const VulkanContext& vulkanContext);
	~DescriptorLayoutCache();

	DescriptorLayoutCache(const DescriptorLayoutCache&) = delete;
	DescriptorLayoutCache& operator=(const DescriptorLayoutCache&) = delete;

	DescriptorLayoutCache(DescriptorLayoutCache&&) = delete;
	DescriptorLayoutCache& operator=(DescriptorLayoutCache&&) = delete;

	VkDescriptorSetLayout CreateLayout(const DescriptorLayout& layoutInfo);

private:
	const VulkanContext* vulkanContext;

	std::unordered_map<DescriptorLayout, VkDescriptorSetLayout> cache;
};