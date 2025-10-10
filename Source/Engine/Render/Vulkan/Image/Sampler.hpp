#pragma once

#include <volk.h>

class VulkanContext;

struct SamplerDescription
{
    VkFilter filter = VK_FILTER_LINEAR;
    VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerReductionModeEXT reductionMode = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE_EXT;
    float maxAnisotropy = 1.0f;
    float minLod = 0.0f;
    float maxLod = 0.0f;
};

class Sampler
{
public:
    Sampler() = default;
    Sampler(SamplerDescription description, const VulkanContext& vulkanContext);

    ~Sampler();

    Sampler(const Sampler&) = delete;
    Sampler& operator=(const Sampler&) = delete;

    Sampler(Sampler&& other) noexcept;
    Sampler& operator=(Sampler&& other) noexcept;
    
    const SamplerDescription& GetDescription() const
    {
        return description;
    }
    
    bool IsValid() const
    {
        return sampler != VK_NULL_HANDLE;
    }
    
    operator VkSampler() const
    {
        return sampler;
    }

private:
    const VulkanContext* vulkanContext = nullptr;

    VkSampler sampler = VK_NULL_HANDLE;
    
    SamplerDescription description = {};
};
