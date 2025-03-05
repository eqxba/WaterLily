#include "Engine/Render/Vulkan/Image/RenderTarget.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

RenderTarget::RenderTarget(ImageDescription description,
    const VkImageAspectFlags aspectFlags, const VulkanContext& vulkanContext)
    : image{ std::move(description), vulkanContext }
    , view{ image, aspectFlags, vulkanContext }
{}

RenderTarget::RenderTarget(const VkImage aImage, ImageDescription description, const VkImageAspectFlags aspectFlags,
    const VulkanContext& vulkanContext, const bool isSwapchainImage /* = false */)
    : image{ aImage, std::move(description), vulkanContext, isSwapchainImage }
    , view{ image, aspectFlags, vulkanContext}
{}
