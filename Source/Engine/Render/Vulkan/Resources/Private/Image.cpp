#include "Engine/Render/Vulkan/Resources/Image.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/Buffer.hpp"

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
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = description.format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = description.usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = 0; // Optional

        return vulkanContext.GetMemoryManager().CreateImage(imageInfo, description.memoryProperties);
    }

    static void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
        const VulkanContext& vulkanContext)
    {
        VkAccessFlags srcAccessMask;
        VkAccessFlags dstAccessMask;
        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
        {
            srcAccessMask = 0;
            dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL 
            && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
        {
            srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else 
        {
            Assert(false);
        }

        const auto commands = [&](VkCommandBuffer commandBuffer) {
            
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = srcAccessMask;
            barrier.dstAccessMask = dstAccessMask;

            vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier); 
        };

        vulkanContext.GetDevice().ExecuteOneTimeCommandBuffer(commands);
    }

    static void CopyBufferToImage(VkBuffer buffer, VkImage image, const Extent2D extent,
        const VulkanContext& vulkanContext)
    {
        vulkanContext.GetDevice().ExecuteOneTimeCommandBuffer([&](VkCommandBuffer commandBuffer) {
            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;

            region.imageOffset = { 0, 0, 0 };
            region.imageExtent = { static_cast<uint32_t>(extent.width), static_cast<uint32_t>(extent.height), 1 };

            vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        });
    }   
}

Image::Image(ImageDescription aDescription, const VulkanContext* aVulkanContext)
    : vulkanContext{ aVulkanContext }
    , description{ aDescription }
{    
    using namespace ImageDetails;

    Assert(vulkanContext != nullptr); 

    image = CreateImage(description, *vulkanContext);    
}

Image::~Image()
{
    if (image != VK_NULL_HANDLE)
    {
        vulkanContext->GetMemoryManager().DestroyImage(image);
    }
}

// TODO: Use custom swap to DRY between 2 below
Image::Image(Image&& other) noexcept
    : vulkanContext{ other.vulkanContext }
    , description{ other.description }
    , image{ other.image }
{
    other.description = {};
    other.image = VK_NULL_HANDLE;    
}

Image& Image::operator=(Image&& other) noexcept
{
    if (this != &other)
    {
        vulkanContext = other.vulkanContext;
        description = other.description;
        image = other.image;

        other.description = {};
        other.image = VK_NULL_HANDLE;       
    }
    return *this;
}

void Image::Fill(const Buffer& buffer) const
{
    using namespace ImageDetails;

    Assert(description.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT);

    // TODO: Remember layout before and do out-and-in transition?
    TransitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    CopyBufferToImage(buffer.GetVkBuffer(), image, description.extent, *vulkanContext);
    TransitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void Image::TransitionLayout(VkImageLayout oldLayout, VkImageLayout newLayout) const
{
    ImageDetails::TransitionImageLayout(image, description.format, oldLayout, newLayout, *vulkanContext);
}

VkImageView Image::CreateImageView(VkImageAspectFlags aspectFlags) const
{
    VkDevice device = vulkanContext->GetDevice().GetVkDevice();
    return VulkanHelpers::CreateImageView(device, image, description.format, aspectFlags);
}