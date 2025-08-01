#include "Engine/Render/Vulkan/Managers/DescriptorSetManager.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"

DescriptorSetManager::DescriptorSetManager(const VulkanContext& aVulkanContext)
    : vulkanContext{aVulkanContext}
    , cache{vulkanContext}
{
    using namespace VulkanConfig;

    allocators[DescriptorScope::eGlobal] = DescriptorSetAllocator(maxSetsInPool, defaultPoolSizeRatios, vulkanContext);
    allocators[DescriptorScope::eSceneRenderer] = DescriptorSetAllocator(maxSetsInPool, defaultPoolSizeRatios, vulkanContext);
    allocators[DescriptorScope::eUiRenderer] = DescriptorSetAllocator(maxSetsInPool, defaultPoolSizeRatios, vulkanContext);
    allocators[DescriptorScope::eComputeRenderer] = DescriptorSetAllocator(maxSetsInPool, defaultPoolSizeRatios, vulkanContext);
}

DescriptorSetManager::~DescriptorSetManager() = default;

DescriptorSetLayoutBuilder DescriptorSetManager::GetDescriptorSetLayoutBuilder()
{
    return DescriptorSetLayoutBuilder(cache);
}

DescriptorSetBuilder DescriptorSetManager::GetDescriptorSetBuilder(
    const DescriptorScope scope /* = DescriptorScope::eGlobal */)
{
    Assert(allocators.contains(scope));
    return { allocators.at(scope), DescriptorSetLayoutBuilder(cache), vulkanContext};
}

DescriptorSetBuilder DescriptorSetManager::GetDescriptorSetBuilder(const DescriptorSetLayout layout, 
    const DescriptorScope scope /* = DescriptorScope::eGlobal */)
{
    Assert(allocators.contains(scope));
    return { allocators.at(scope), layout, vulkanContext };
}

ReflectiveDescriptorSetBuilder DescriptorSetManager::GetReflectiveDescriptorSetBuilder(const Pipeline& pipeline,
    const DescriptorScope scope /* = DescriptorScope::eGlobal */)
{
    Assert(pipeline.IsValid());
    Assert(allocators.contains(scope));
    return { allocators.at(scope), pipeline.GetSetReflections(), pipeline.GetSetLayouts(), vulkanContext };
}

void DescriptorSetManager::ResetDescriptors(const DescriptorScope scope)
{
    allocators[scope].ResetPools();
}
