#include "Engine/Render/Resources/DescriptorManager.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"

DescriptorManager::DescriptorManager(const VulkanContext& aVulkanContext)
    : vulkanContext{aVulkanContext}
    , cache{vulkanContext}
{
	allocators[DescriptorScope::eGlobal] = DescriptorAllocator(vulkanContext, VulkanConfig::maxSetsInPool,
		VulkanConfig::defaultPoolSizeRatios);
	allocators[DescriptorScope::eScene] = DescriptorAllocator(vulkanContext, VulkanConfig::maxSetsInPool,
		VulkanConfig::defaultPoolSizeRatios);
}

DescriptorManager::~DescriptorManager() = default;

DescriptorBuilder DescriptorManager::GetDescriptorBuilder(
	const DescriptorScope scope /* = DescriptorScope::eGlobal */)
{
	Assert(allocators.contains(scope));
	return { vulkanContext, cache, allocators[scope] };
}

void DescriptorManager::ResetDescriptors(const DescriptorScope scope)
{
	allocators[scope].ResetPools();
}