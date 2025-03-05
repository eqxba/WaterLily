#include "Engine/Render/Vulkan/Image/Sampler.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace SamplerDetails
{
    static VkSampler CreateSampler(const SamplerDescription& description, const VulkanContext& vulkanContext)
    {
        // TODO: More parameters to description
        const VkSamplerCreateInfo samplerInfo = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = description.addressMode[0],
            .addressModeV = description.addressMode[1],
            .addressModeW = description.addressMode[2],
            .mipLodBias = 0.0f,
            .anisotropyEnable = VK_TRUE,
            .maxAnisotropy = description.maxAnisotropy,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .minLod = description.minLod,
            .maxLod = description.maxLod,
            .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
            .unnormalizedCoordinates = VK_FALSE, };

        VkSampler sampler;
        VkResult result = vkCreateSampler(vulkanContext.GetDevice(), &samplerInfo, nullptr, &sampler);
        Assert(result == VK_SUCCESS);

        return sampler;
    }
}

Sampler::Sampler(SamplerDescription aDescription, const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
    , sampler{ SamplerDetails::CreateSampler(aDescription, aVulkanContext) }
    , description{ std::move(aDescription) }
{}

Sampler::~Sampler()
{
    if (IsValid())
    {
        vkDestroySampler(vulkanContext->GetDevice(), sampler, nullptr);
        sampler = VK_NULL_HANDLE;
    }
}

Sampler::Sampler(Sampler&& other) noexcept
    : vulkanContext{ other.vulkanContext }
    , sampler{ other.sampler }
    , description{ std::move(other.description) }
{
    other.vulkanContext = nullptr;
    other.sampler = VK_NULL_HANDLE;
    other.description = {};
}

Sampler& Sampler::operator=(Sampler&& other) noexcept
{
    if (this != &other)
    {
        std::swap(vulkanContext, other.vulkanContext);
        std::swap(sampler, other.sampler);
        std::swap(description, other.description);
    }
    
    return *this;
}
