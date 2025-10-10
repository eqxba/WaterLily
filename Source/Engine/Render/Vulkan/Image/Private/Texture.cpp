#include "Engine/Render/Vulkan/Image/Texture.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

Texture::Texture(ImageDescription imageDescription, const VulkanContext& vulkanContext)
    : image{ std::move(imageDescription), vulkanContext }
    , view{ image, VK_IMAGE_ASPECT_COLOR_BIT, 0, imageDescription.mipLevelsCount, vulkanContext }
{}

Texture::Texture(ImageDescription imageDescription, SamplerDescription samplerDescription,
    const VulkanContext& vulkanContext)
    : Texture{ std::move(imageDescription), vulkanContext }
{
    sampler = Sampler(std::move(samplerDescription), vulkanContext);
}
