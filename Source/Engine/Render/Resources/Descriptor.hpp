#pragma once

#include <volk.h>

class VulkanContext;
class DescriptorLayoutCache;
class DescriptorAllocator;
class Buffer;
class ImageView;

struct Descriptor
{
	VkDescriptorSetLayout layout = VK_NULL_HANDLE;
	VkDescriptorSet set = VK_NULL_HANDLE;
};

class DescriptorLayout
{
public:
	void AddBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags shaderStages);

	bool operator==(const DescriptorLayout& other) const;

	const std::vector<VkDescriptorSetLayoutBinding>& GetBindings() const
	{
		return bindings;
	}

private:
	std::vector<VkDescriptorSetLayoutBinding> bindings;
};

class DescriptorBuilder
{
public:
	DescriptorBuilder(const VulkanContext& vulkanContext, DescriptorLayoutCache& cache, DescriptorAllocator& allocator);

	DescriptorBuilder& Bind(uint32_t binding, const Buffer& buffer, VkDescriptorType type, VkShaderStageFlags shaderStages);
	DescriptorBuilder& Bind(uint32_t binding, const ImageView& imageView, VkDescriptorType type, VkShaderStageFlags shaderStages);
	DescriptorBuilder& Bind(uint32_t binding, VkSampler sampler, VkShaderStageFlags shaderStages);

	Descriptor Build();

private:
	VkWriteDescriptorSet& UpdateLayoutAndCreateWrite(uint32_t binding, VkDescriptorType type,
		VkShaderStageFlags shaderStages);

	const VulkanContext* vulkanContext;

	DescriptorLayoutCache* cache;
	DescriptorAllocator* allocator;

	DescriptorLayout layout;

	std::vector<VkDescriptorBufferInfo> bufferInfos;
	std::vector<VkDescriptorImageInfo> imageInfos;

	std::vector<VkWriteDescriptorSet> descriptorWrites;
};

bool operator==(const VkDescriptorSetLayoutBinding& lhs, const VkDescriptorSetLayoutBinding& rhs);

template <>
struct std::hash<VkDescriptorSetLayoutBinding>
{
	std::size_t operator()(const VkDescriptorSetLayoutBinding& binding) const noexcept
	{
		const std::size_t h1 = std::hash<uint32_t>{}(binding.binding);
		const std::size_t h2 = std::hash<uint32_t>{}(binding.descriptorType);
		const std::size_t h3 = std::hash<uint32_t>{}(binding.descriptorCount);
		const std::size_t h4 = std::hash<uint32_t>{}(binding.stageFlags);
		const std::size_t h5 = std::hash<const void*>{}(binding.pImmutableSamplers);

		return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4);
	}
};

template <>
struct std::hash<DescriptorLayout>
{
	std::size_t operator()(const DescriptorLayout& info) const noexcept
	{
		const std::vector<VkDescriptorSetLayoutBinding>& bindings = info.GetBindings();

		std::size_t seed = bindings.size();

		std::ranges::for_each(bindings, [&seed](const auto& binding) {
			seed ^= std::hash<VkDescriptorSetLayoutBinding>{}(binding)+0x9e3779b9 + (seed << 6) + (seed >> 2);
		});

		return seed;
	}
};