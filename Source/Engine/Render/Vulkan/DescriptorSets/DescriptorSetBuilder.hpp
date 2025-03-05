#pragma once

#include <volk.h>

#include "Engine/Render/Vulkan/DescriptorSets/DescriptorSetLayout.hpp"
#include "Engine/Render/Vulkan/DescriptorSets/DescriptorSetLayoutBuilder.hpp"

class VulkanContext;
class DescriptorSetAllocator;
class Buffer;
class ImageView;
struct RenderTarget;
struct Texture;

class DescriptorSetBuilder
{
public:
    DescriptorSetBuilder(DescriptorSetAllocator& allocator, DescriptorSetLayoutBuilder layoutBuilder, const VulkanContext& vulkanContext);
    DescriptorSetBuilder(DescriptorSetAllocator& allocator, DescriptorSetLayout layout, const VulkanContext& vulkanContext);

    std::tuple<VkDescriptorSet, DescriptorSetLayout> Build();

    // API for building layout and binding resources
    
    // Some helpers for high-level resources
    DescriptorSetBuilder& Bind(uint32_t binding, const RenderTarget& renderTarget, VkImageLayout layout, VkShaderStageFlags shaderStages);
    DescriptorSetBuilder& Bind(uint32_t binding, const Texture& texture, VkShaderStageFlags shaderStages);
    
    DescriptorSetBuilder& Bind(uint32_t binding, const Buffer& buffer, VkDescriptorType type, VkShaderStageFlags shaderStages);
    DescriptorSetBuilder& Bind(uint32_t binding, const ImageView& imageView, VkImageLayout layout, VkDescriptorType type, VkShaderStageFlags shaderStages);
    DescriptorSetBuilder& Bind(uint32_t binding, VkSampler sampler, VkShaderStageFlags shaderStages);
    DescriptorSetBuilder& Bind(uint32_t binding, const ImageView& imageView, VkImageLayout layout, VkSampler sampler, VkShaderStageFlags shaderStages);

    // API for binding resources when we already have the layout
    
    // Some helpers for high-level resources
    DescriptorSetBuilder& Bind(uint32_t binding, const RenderTarget& renderTarget, VkImageLayout layout);
    DescriptorSetBuilder& Bind(uint32_t binding, const Texture& texture);
    
    DescriptorSetBuilder& Bind(uint32_t binding, const Buffer& buffer);
    DescriptorSetBuilder& Bind(uint32_t binding, const ImageView& imageView, VkImageLayout layout);
    DescriptorSetBuilder& Bind(uint32_t binding, VkSampler sampler);
    DescriptorSetBuilder& Bind(uint32_t binding, const ImageView& imageView, VkImageLayout layout, VkSampler sampler);

private:
    const VkDescriptorSetLayoutBinding& GetBinding(uint32_t binding) const;
    void AddBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags shaderStages);
    VkWriteDescriptorSet& CreateWrite(uint32_t binding);

    const VulkanContext* vulkanContext;

    DescriptorSetAllocator* allocator;

    DescriptorSetLayout layout;
    std::optional<DescriptorSetLayoutBuilder> layoutBuilder;

    std::vector<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkDescriptorImageInfo> imageInfos;

    std::vector<VkWriteDescriptorSet> descriptorWrites;
};
