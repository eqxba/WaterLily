#pragma once

#include "Engine/Render/Resources/Descriptor.hpp"
#include "Engine/Render/Resources/DescriptorAllocator.hpp"
#include "Engine/Render/Resources/DescriptorLayoutCache.hpp"

class VulkanContext;

enum class DescriptorScope
{
    eGlobal,
	eScene,
};

class DescriptorManager
{
public:
	explicit DescriptorManager(const VulkanContext& vulkanContext);
	~DescriptorManager();

	DescriptorManager(const DescriptorManager&) = delete;
	DescriptorManager& operator=(const DescriptorManager&) = delete;

	DescriptorManager(DescriptorManager&&) = delete;
	DescriptorManager& operator=(DescriptorManager&&) = delete;

	DescriptorBuilder GetDescriptorBuilder(DescriptorScope scope = DescriptorScope::eGlobal);

	void ResetDescriptors(DescriptorScope scope);

private:
	const VulkanContext& vulkanContext;

	DescriptorLayoutCache cache;

	std::unordered_map<DescriptorScope, DescriptorAllocator> allocators;
};