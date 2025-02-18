#pragma once

#include "Engine/Render/Resources/DescriptorSets/DescriptorSetLayoutBuilder.hpp"
#include "Engine/Render/Resources/DescriptorSets/DescriptorSetBuilder.hpp"
#include "Engine/Render/Resources/DescriptorSets/DescriptorSetAllocator.hpp"
#include "Engine/Render/Resources/DescriptorSets/DescriptorSetLayoutCache.hpp"

class VulkanContext;

enum class DescriptorScope
{
    eGlobal,
    eScene,
};

class DescriptorSetManager
{
public:
    explicit DescriptorSetManager(const VulkanContext& vulkanContext);
    ~DescriptorSetManager();

    DescriptorSetManager(const DescriptorSetManager&) = delete;
    DescriptorSetManager& operator=(const DescriptorSetManager&) = delete;

    DescriptorSetManager(DescriptorSetManager&&) = delete;
    DescriptorSetManager& operator=(DescriptorSetManager&&) = delete;

    DescriptorSetLayoutBuilder GetDescriptorSetLayoutBuilder();

    DescriptorSetBuilder GetDescriptorSetBuilder(DescriptorScope scope = DescriptorScope::eGlobal);
    DescriptorSetBuilder GetDescriptorSetBuilder(DescriptorSetLayout layout, DescriptorScope scope = DescriptorScope::eGlobal);

    void ResetDescriptors(DescriptorScope scope);

private:
    const VulkanContext& vulkanContext;

    DescriptorSetLayoutCache cache;

    std::unordered_map<DescriptorScope, DescriptorSetAllocator> allocators;
};