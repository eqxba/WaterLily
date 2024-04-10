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
        imageInfo.mipLevels = description.mipLevelsCount;
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

    static void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, 
        VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t mipLevelsCount)
    {
        VkAccessFlags srcAccessMask{};
        VkAccessFlags dstAccessMask{};
        VkPipelineStageFlags sourceStage{};
        VkPipelineStageFlags destinationStage{};

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
        {
            srcAccessMask = 0;
            dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
        {
            srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
        {
            srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {            
            srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else 
        {
            Assert(false);
        }

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = baseMipLevel;
        barrier.subresourceRange.levelCount = mipLevelsCount;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = srcAccessMask;
        barrier.dstAccessMask = dstAccessMask;

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    static void BlitMipToNextMip(VkCommandBuffer commandBuffer, VkImage image, uint32_t mipLevel, int32_t mipWidth, 
        int32_t mipHeight)
    {
        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = mipLevel;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = mipLevel + 1;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
    }

    static void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, const Extent2D extent)
    {
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

void Image::FillMipLevel0(const Buffer& buffer, bool generateOtherMipLevels /* = false */) const
{
    using namespace ImageDetails;

    Assert(description.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT);

    // TODO: Remember layout before and do out-and-in transition?

    vulkanContext->GetDevice().ExecuteOneTimeCommandBuffer([&](VkCommandBuffer commandBuffer) {
        TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        CopyBufferToImage(commandBuffer, buffer.GetVkBuffer(), image, description.extent);

        if (generateOtherMipLevels && description.mipLevelsCount > 1)
        {
            GenerateMipLevelsFromLevel0(commandBuffer);
        }
        else
        {
            TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    });
}

VkImageView Image::CreateImageView(VkImageAspectFlags aspectFlags) const
{
    VkDevice device = vulkanContext->GetDevice().GetVkDevice();
    return VulkanHelpers::CreateImageView(device, image, description.format, aspectFlags, description.mipLevelsCount);
}

void Image::TransitionLayout(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout) const
{
    TransitionLayout(commandBuffer, oldLayout, newLayout, 0, description.mipLevelsCount);
}

void Image::TransitionLayout(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout,
    uint32_t baseMipLevel, uint32_t mipLevelsCount) const
{
    ImageDetails::TransitionImageLayout(commandBuffer, image, oldLayout, newLayout, baseMipLevel, mipLevelsCount);
}

void Image::GenerateMipLevelsFromLevel0(VkCommandBuffer cb) const
{
    Assert(description.usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

    int32_t mipWidth = description.extent.width;
    int32_t mipHeight = description.extent.height;

    std::vector<uint32_t> sourceMips(description.mipLevelsCount - 1);
    std::ranges::iota(sourceMips, 0);

    const auto generateNextMipLevel = [&](uint32_t sourceMip) {
        TransitionLayout(cb, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, sourceMip, 1);
        ImageDetails::BlitMipToNextMip(cb, image, sourceMip, mipWidth, mipHeight);
        TransitionLayout(cb, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sourceMip, 1);

        mipWidth = std::max(mipWidth / 2, 1);
        mipHeight = std::max(mipHeight / 2, 1);
    };

    std::ranges::for_each(sourceMips, generateNextMipLevel);   

    TransitionLayout(cb, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, description.mipLevelsCount - 1, 1);
}