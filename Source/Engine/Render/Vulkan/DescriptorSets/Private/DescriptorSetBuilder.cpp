#include "Engine/Render/Vulkan/DescriptorSets/DescriptorSetBuilder.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/DescriptorSets/DescriptorSetAllocator.hpp"
#include "Engine/Render/Vulkan/Buffer/Buffer.hpp"
#include "Engine/Render/Vulkan/Image/RenderTarget.hpp"
#include "Engine/Render/Vulkan/Image/Texture.hpp"

namespace DescriptorSetBuilderDetails
{
    std::tuple<uint32_t, const BindingReflection*> FindBinding(const std::string_view name,
        const std::vector<DescriptorSetReflection>& setReflections)
    {
        uint32_t setIndex = 0;
        const BindingReflection* bindingReflection = nullptr;
        
        for (uint32_t i = 0; i < setReflections.size(); ++i)
        {
            const DescriptorSetReflection& setReflection = setReflections[i];
            
            const auto it = std::ranges::find_if(setReflection.bindings, [&](const BindingReflection& bindingReflection){
                return bindingReflection.name == name; });
            
            if (it != setReflection.bindings.end())
            {
                setIndex = i;
                bindingReflection = &(*it);
                break;
            }
        }
        
        Assert(bindingReflection != nullptr);
        
        return { setIndex, bindingReflection };
    }
}

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

    const VkDescriptorSet set = allocator->Allocate(layout);

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

    vkUpdateDescriptorSets(vulkanContext->GetDevice(), static_cast<uint32_t>(descriptorWrites.size()),
        descriptorWrites.data(), 0, nullptr);

    return { set, layout };
}

DescriptorSetBuilder& DescriptorSetBuilder::Bind(const uint32_t binding, const RenderTarget& renderTarget,
    const VkImageLayout aLayout, const VkShaderStageFlags shaderStages)
{
    return Bind(binding, renderTarget.view, aLayout, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, shaderStages);
}

DescriptorSetBuilder& DescriptorSetBuilder::Bind(const uint32_t binding, const Texture& texture,
    const VkShaderStageFlags shaderStages)
{
    // For binding textures we always consider that they're in read-only optimal layout
    return Bind(binding, texture.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture.sampler, shaderStages);
}

DescriptorSetBuilder& DescriptorSetBuilder::Bind(const uint32_t binding, const Buffer& buffer, const VkDescriptorType type,
    const VkShaderStageFlags shaderStages)
{
    AddBinding(binding, type, shaderStages);
    Bind(binding, buffer);

    return *this;
}

DescriptorSetBuilder& DescriptorSetBuilder::Bind(const uint32_t binding, const ImageView& imageView,
    const VkImageLayout aLayout, const VkDescriptorType type, const VkShaderStageFlags shaderStages)
{
    AddBinding(binding, type, shaderStages);
    Bind(binding, imageView, aLayout);

    return *this;
}

DescriptorSetBuilder& DescriptorSetBuilder::Bind(const uint32_t binding, const VkSampler sampler,
    const VkShaderStageFlags shaderStages)
{
    AddBinding(binding, VK_DESCRIPTOR_TYPE_SAMPLER, shaderStages);
    Bind(binding, sampler);

    return *this;
}

DescriptorSetBuilder& DescriptorSetBuilder::Bind(const uint32_t binding, const ImageView& imageView, 
    const VkImageLayout aLayout, const VkSampler sampler, const VkShaderStageFlags shaderStages)
{
    AddBinding(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, shaderStages);
    Bind(binding, imageView, aLayout, sampler);

    return *this;
}

DescriptorSetBuilder& DescriptorSetBuilder::Bind(const uint32_t binding, const RenderTarget& renderTarget,
    const VkImageLayout aLayout)
{
    return Bind(binding, renderTarget.view, aLayout);
}

DescriptorSetBuilder& DescriptorSetBuilder::Bind(const uint32_t binding, const Texture& texture)
{
    // For binding textures we always consider that they're in read-only optimal layout
    return Bind(binding, texture.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture.sampler);
}

DescriptorSetBuilder& DescriptorSetBuilder::Bind(const uint32_t binding, const Buffer& buffer)
{
    bufferInfos.emplace_back(buffer, 0, buffer.GetDescription().size);

    // Hack: store index (1-based) of the element in the bufferInfos vector and resolve it to the actual pointer in the
    // Build() function to handle possible buffer reallocations
    VkWriteDescriptorSet& bufferWrite = CreateWrite(binding);
    bufferWrite.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(bufferInfos.size());

    return *this;
}

DescriptorSetBuilder& DescriptorSetBuilder::Bind(const uint32_t binding, const ImageView& imageView,
    const VkImageLayout aLayout)
{
    imageInfos.emplace_back(VK_NULL_HANDLE, imageView, aLayout);

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

DescriptorSetBuilder& DescriptorSetBuilder::Bind(const uint32_t binding, const ImageView& imageView, 
    const VkImageLayout aLayout, const VkSampler sampler)
{
    imageInfos.emplace_back(sampler, imageView, aLayout);

    // Hack: store index (1-based) of the element in the imageInfos vector and resolve it to the actual pointer in the
    // Build() function to handle possible buffer reallocations
    VkWriteDescriptorSet& imageWrite = CreateWrite(binding);
    imageWrite.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(imageInfos.size());

    return *this;
}

const VkDescriptorSetLayoutBinding& DescriptorSetBuilder::GetBinding(const uint32_t binding) const
{
    return layoutBuilder ? layoutBuilder->GetBinding(binding) : layout.GetBinding(binding);
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

ReflectiveDescriptorSetBuilder::ReflectiveDescriptorSetBuilder(DescriptorSetAllocator& aAllocator,
    const std::vector<DescriptorSetReflection>& aSetReflections, const std::vector<DescriptorSetLayout>& aSetLayouts,
    const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
    , allocator{ &aAllocator }
    , setReflections{ &aSetReflections }
    , setLayouts{ &aSetLayouts }
{
    Assert(!setReflections->empty());
    Assert(setReflections->size() == setLayouts->size());
}

std::vector<VkDescriptorSet> ReflectiveDescriptorSetBuilder::Build()
{
    std::vector<VkDescriptorSet> descriptors;
    descriptors.reserve(setBuilders.size());
    
    for (auto& [setIndex, builder] : setBuilders)
    {
        const auto& [descriptor, layout] = builder.Build();
        descriptors.push_back(descriptor);
    }
    
    return descriptors;
}

ReflectiveDescriptorSetBuilder& ReflectiveDescriptorSetBuilder::Bind(const std::string_view name,
    const RenderTarget& renderTarget, VkImageLayout layout)
{
    const auto [builder, bindingReflection] = GetBuilderAndBinding(name);
    builder->Bind(bindingReflection->index, renderTarget, layout);
    
    return *this;
}

ReflectiveDescriptorSetBuilder& ReflectiveDescriptorSetBuilder::Bind(const std::string_view name, const Texture& texture)
{
    const auto [builder, bindingReflection] = GetBuilderAndBinding(name);
    builder->Bind(bindingReflection->index, texture);
    
    return *this;
}

ReflectiveDescriptorSetBuilder& ReflectiveDescriptorSetBuilder::Bind(const std::string_view name, const Buffer& buffer)
{
    const auto [builder, bindingReflection] = GetBuilderAndBinding(name);
    builder->Bind(bindingReflection->index, buffer);
    
    return *this;
}

ReflectiveDescriptorSetBuilder& ReflectiveDescriptorSetBuilder::Bind(const std::string_view name,
    const ImageView& imageView, VkImageLayout layout)
{
    const auto [builder, bindingReflection] = GetBuilderAndBinding(name);
    builder->Bind(bindingReflection->index, imageView, layout);
    
    return *this;
}

ReflectiveDescriptorSetBuilder& ReflectiveDescriptorSetBuilder::Bind(const std::string_view name, const VkSampler sampler)
{
    const auto [builder, bindingReflection] = GetBuilderAndBinding(name);
    builder->Bind(bindingReflection->index, sampler);
    
    return *this;
}

ReflectiveDescriptorSetBuilder& ReflectiveDescriptorSetBuilder::Bind(const std::string_view name,
    const ImageView& imageView, const VkImageLayout layout, const VkSampler sampler)
{
    const auto [builder, bindingReflection] = GetBuilderAndBinding(name);
    builder->Bind(bindingReflection->index, imageView, layout, sampler);
    
    return *this;
}

std::tuple<DescriptorSetBuilder*, const BindingReflection*> ReflectiveDescriptorSetBuilder::GetBuilderAndBinding(
    const std::string_view name)
{
    const auto [setIndex, bindingReflection] = DescriptorSetBuilderDetails::FindBinding(name, *setReflections);
    
    if (!setBuilders.contains(setIndex))
    {
        setBuilders.emplace(setIndex, DescriptorSetBuilder(*allocator, (*setLayouts)[setIndex], *vulkanContext));
    }
    
    return { &setBuilders.at(setIndex), bindingReflection };
}
