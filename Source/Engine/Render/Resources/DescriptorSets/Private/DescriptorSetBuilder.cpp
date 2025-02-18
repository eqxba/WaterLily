#include "Engine/Render/Resources/DescriptorSets/DescriptorSetBuilder.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Resources/DescriptorSets/DescriptorSetAllocator.hpp"
#include "Engine/Render/Resources/Buffer.hpp"
#include "Engine/Render/Resources/ImageView.hpp"

DescriptorSetBuilder::DescriptorSetBuilder(DescriptorSetAllocator& aAllocator, DescriptorSetLayoutBuilder aLayoutBuilder,
    const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
    , allocator{ &aAllocator }
    , layoutBuilder{ std::move(aLayoutBuilder) }
{}

DescriptorSetBuilder::DescriptorSetBuilder(DescriptorSetAllocator& aAllocator, const DescriptorSetLayout aLayout,
    const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
    , allocator{ &aAllocator }
    , layout{ aLayout }
{
    Assert(layout.IsValid());
}

std::tuple<VkDescriptorSet, DescriptorSetLayout> DescriptorSetBuilder::Build()
{
    if (!layout.IsValid())
    {
        Assert(layoutBuilder);
        layout = layoutBuilder->Build();
    }

    const VkDescriptorSet set = allocator->Allocate(layout.GetVkDescriptorSetLayout());

    // Hack: resolve index to pointers (see appropriate bind functions)
    std::ranges::for_each(descriptorWrites, [&](VkWriteDescriptorSet& descriptorWrite) {
        descriptorWrite.dstSet = set;

        if (const auto bufferInfosIndex = reinterpret_cast<size_t>(descriptorWrite.pBufferInfo); bufferInfosIndex != 0)
        {
            descriptorWrite.pBufferInfo = &bufferInfos[bufferInfosIndex - 1];
        }

        if (const auto imageInfosIndex = reinterpret_cast<size_t>(descriptorWrite.pImageInfo); imageInfosIndex != 0)
        {
            descriptorWrite.pImageInfo = &imageInfos[imageInfosIndex - 1];
        }
    });

    vkUpdateDescriptorSets(vulkanContext->GetDevice().GetVkDevice(), static_cast<uint32_t>(descriptorWrites.size()),
        descriptorWrites.data(), 0, nullptr);

    return { set, layout };
}

DescriptorSetBuilder& DescriptorSetBuilder::Bind(const uint32_t binding, const Buffer& buffer, const VkDescriptorType type,
    const VkShaderStageFlags shaderStages)
{
    AddBinding(binding, type, shaderStages);
    Bind(binding, buffer);

    return *this;
}

DescriptorSetBuilder& DescriptorSetBuilder::Bind(const uint32_t binding, const ImageView& imageView,
    const VkDescriptorType type, const VkShaderStageFlags shaderStages)
{
    AddBinding(binding, type, shaderStages);
    Bind(binding, imageView);

    return *this;
}

DescriptorSetBuilder& DescriptorSetBuilder::Bind(const uint32_t binding, const VkSampler sampler,
    const VkShaderStageFlags shaderStages)
{
    AddBinding(binding, VK_DESCRIPTOR_TYPE_SAMPLER, shaderStages);
    Bind(binding, sampler);

    return *this;
}

DescriptorSetBuilder& DescriptorSetBuilder::Bind(const uint32_t binding, const Buffer& buffer)
{
    bufferInfos.emplace_back(buffer.GetVkBuffer(), 0, buffer.GetDescription().size);

    // Hack: store index (1-based) of the element in the bufferInfos vector and resolve it to the actual pointer in the
    // Build() function to handle possible buffer reallocations
    VkWriteDescriptorSet& bufferWrite = CreateWrite(binding);
    bufferWrite.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(bufferInfos.size());

    return *this;
}

DescriptorSetBuilder& DescriptorSetBuilder::Bind(const uint32_t binding, const ImageView& imageView)
{
    // TODO: Get rid of hardcoded layout here as we will need to write to images in shaders
    imageInfos.emplace_back(VK_NULL_HANDLE, imageView.GetVkImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Hack: store index (1-based) of the element in the imageInfos vector and resolve it to the actual pointer in the
    // Build() function to handle possible buffer reallocations
    VkWriteDescriptorSet& imageWrite = CreateWrite(binding);
    imageWrite.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(imageInfos.size());

    return *this;
}

DescriptorSetBuilder& DescriptorSetBuilder::Bind(const uint32_t binding, VkSampler sampler)
{
    imageInfos.emplace_back(sampler, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED);

    // Hack: store index (1-based) of the element in the imageInfos vector and resolve it to the actual pointer in the
    // Build() function to handle possible buffer reallocations
    VkWriteDescriptorSet& imageWrite = CreateWrite(binding);
    imageWrite.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(imageInfos.size());

    return *this;
}

const VkDescriptorSetLayoutBinding& DescriptorSetBuilder::GetBinding(const uint32_t index) const
{
    return layoutBuilder ? layoutBuilder->GetBinding(index) : layout.GetBinding(index);
}

void DescriptorSetBuilder::AddBinding(const uint32_t binding, const VkDescriptorType descriptorType, 
    const VkShaderStageFlags shaderStages)
{
    Assert(layoutBuilder);
    layoutBuilder->AddBinding(binding, descriptorType, shaderStages);
}

VkWriteDescriptorSet& DescriptorSetBuilder::CreateWrite(const uint32_t binding)
{
    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.pNext = nullptr;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.descriptorType = GetBinding(binding).descriptorType;
    descriptorWrite.dstBinding = binding;

    descriptorWrites.push_back(descriptorWrite);

    return descriptorWrites.back();
}