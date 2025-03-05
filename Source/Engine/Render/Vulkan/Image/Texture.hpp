#pragma once

#include <volk.h>

#include "Engine/Render/Vulkan/Image/Sampler.hpp"
#include "Engine/Render/Vulkan/Image/ImageView.hpp"

class VulkanContext;

struct Texture
{
    Texture() = default;
    Texture(ImageDescription imageDescription, const VulkanContext& vulkanContext);
    Texture(ImageDescription imageDescription, SamplerDescription samplerDescription,
        const VulkanContext& vulkanContext);
    
    bool IsValid() const
    {
        return image.IsValid() && view.IsValid();
    }
    
    operator const Image&() const
    {
        return image;
    }
    
    Image image;
    ImageView view;
    Sampler sampler;
};
