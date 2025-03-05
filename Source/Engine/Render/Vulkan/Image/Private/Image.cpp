#include "Engine/Render/Vulkan/Image/Image.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace ImageDetails
{
    static VkImage CreateImage(const ImageDescription& description, const VulkanContext& vulkanContext)
    {
        // TODO: move more of these as parameters
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<uint32_t>(description.extent.width);
        imageInfo.extent.height = static_cast<uint32_t>(description.extent.height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = description.mipLevelsCount;
        imageInfo.arrayLayers = 1;
        imageInfo.format = description.format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = description.usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = description.samples;
        imageInfo.flags = 0; // Optional

        return vulkanContext.GetMemoryManager().CreateImage(imageInfo, description.memoryProperties);
    }
}

Image::Image(VkImage aImage, ImageDescription aDescription, const VulkanContext& aVulkanContext,
    const bool aIsSwapchainImage /* = false */)
    : vulkanContext{ &aVulkanContext }
    , image{ aImage }
    , description{ std::move(aDescription) }
    , isSwapchainImage{ aIsSwapchainImage }
{
    Assert(image != VK_NULL_HANDLE);
}

Image::Image(ImageDescription aDescription, const VulkanContext& aVulkanContext)
    : Image(ImageDetails::CreateImage(aDescription, aVulkanContext), std::move(aDescription), aVulkanContext, false)
{}

Image::~Image()
{
    if (IsValid() && !isSwapchainImage)
    {
        vulkanContext->GetMemoryManager().DestroyImage(image);
        image = VK_NULL_HANDLE;
    }
}

Image::Image(Image&& other) noexcept
    : vulkanContext{ other.vulkanContext }
    , image{ other.image }
    , description{ std::move(other.description) }
    , isSwapchainImage{ other.isSwapchainImage }
{
    other.vulkanContext = nullptr;
    other.image = VK_NULL_HANDLE;
    other.description = {};
    other.isSwapchainImage = false;
}

Image& Image::operator=(Image&& other) noexcept
{
    if (this != &other)
    {
        std::swap(vulkanContext, other.vulkanContext);
        std::swap(image, other.image);
        std::swap(description, other.description);
        std::swap(isSwapchainImage, other.isSwapchainImage);
    }
    
    return *this;
}
