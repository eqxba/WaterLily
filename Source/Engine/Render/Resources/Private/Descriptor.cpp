#include "Engine/Render/Resources/Descriptor.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Resources/DescriptorAllocator.hpp"
#include "Engine/Render/Resources/DescriptorLayoutCache.hpp"
#include "Engine/Render/Resources/Buffer.hpp"
#include "Engine/Render/Resources/ImageView.hpp"

void DescriptorLayout::AddBinding(const uint32_t aBinding, const VkDescriptorType descriptorType,
	const VkShaderStageFlags shaderStages)
{
	std::ranges::for_each(bindings, [&](const auto& binding) {
		Assert(binding.binding != aBinding);
	});

	VkDescriptorSetLayoutBinding binding{};
	binding.binding = aBinding;
	binding.descriptorCount = 1;
	binding.descriptorType = descriptorType;
	binding.stageFlags = shaderStages;

	bindings.push_back(binding);

	std::ranges::sort(bindings, [](const auto& lhs, const auto& rhs) {
		return lhs.binding < rhs.binding;
	});
}

bool DescriptorLayout::operator==(const DescriptorLayout& other) const
{
	return bindings == other.bindings;
}

DescriptorBuilder::DescriptorBuilder(const VulkanContext& aVulkanContext, DescriptorLayoutCache& aCache,
	DescriptorAllocator& aAllocator)
	: vulkanContext{ &aVulkanContext }
	, cache{ &aCache }
	, allocator{ &aAllocator }
{}

DescriptorBuilder& DescriptorBuilder::Bind(const uint32_t binding, const Buffer& buffer, const VkDescriptorType type,
	const VkShaderStageFlags shaderStages)
{
	bufferInfos.emplace_back(buffer.GetVkBuffer(), 0, buffer.GetDescription().size);

	// Hack: store index (1-based) of the element in the bufferInfos vector and resolve it to the actual pointer in the
	// Build() function to handle possible buffer reallocations
	VkWriteDescriptorSet& bufferWrite = UpdateLayoutAndCreateWrite(binding, type, shaderStages);
	bufferWrite.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(bufferInfos.size());

	return *this;
}

DescriptorBuilder& DescriptorBuilder::Bind(const uint32_t binding, const ImageView& imageView,
	const VkDescriptorType type, const VkShaderStageFlags shaderStages)
{
	// TODO: Get rid of hardcoded layout here as we will need to write to images in shaders
	imageInfos.emplace_back(VK_NULL_HANDLE, imageView.GetVkImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Hack: store index (1-based) of the element in the imageInfos vector and resolve it to the actual pointer in the
	// Build() function to handle possible buffer reallocations
	VkWriteDescriptorSet& imageWrite = UpdateLayoutAndCreateWrite(binding, type, shaderStages);
	imageWrite.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(imageInfos.size());

	return *this;
}

DescriptorBuilder& DescriptorBuilder::Bind(const uint32_t binding, const VkSampler sampler,
	const VkShaderStageFlags shaderStages)
{
	imageInfos.emplace_back(sampler, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED);

	// Hack: store index (1-based) of the element in the imageInfos vector and resolve it to the actual pointer in the
	// Build() function to handle possible buffer reallocations
	VkWriteDescriptorSet& imageWrite = UpdateLayoutAndCreateWrite(binding, VK_DESCRIPTOR_TYPE_SAMPLER, shaderStages);
	imageWrite.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(imageInfos.size());

	return *this;
}

Descriptor DescriptorBuilder::Build()
{
	const VkDescriptorSetLayout setLayout = cache->CreateLayout(layout);
	const VkDescriptorSet set = allocator->Allocate(setLayout);

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

	return { setLayout, set };
}

VkWriteDescriptorSet& DescriptorBuilder::UpdateLayoutAndCreateWrite(const uint32_t binding, const VkDescriptorType type,
	const VkShaderStageFlags shaderStages)
{
	layout.AddBinding(binding, type, shaderStages);

	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.pNext = nullptr;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.descriptorType = type;
	descriptorWrite.dstBinding = binding;

	descriptorWrites.push_back(descriptorWrite);

	return descriptorWrites.back();
}

bool operator==(const VkDescriptorSetLayoutBinding& lhs, const VkDescriptorSetLayoutBinding& rhs)
{
	return lhs.binding == rhs.binding
		&& lhs.descriptorType == rhs.descriptorType
		&& lhs.descriptorCount == rhs.descriptorCount
		&& lhs.stageFlags == rhs.stageFlags
		&& lhs.pImmutableSamplers == rhs.pImmutableSamplers;
}