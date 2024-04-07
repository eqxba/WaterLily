#include "Engine/Render/Vulkan/Resources/Image.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/Buffer.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace ImageDetails
{
    static BufferDescription GetStagingBufferDescription(const VkDeviceSize imageSize)
    {
        BufferDescription bufferDescription = { .size = imageSize,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

        return bufferDescription;
    }

    static VkImage CreateImage(const ImageDescription& description, const Extent2D extent, 
        const VulkanContext& vulkanContext)
    {
        // TODO: move more of these as parameters
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<uint32_t>(extent.width);
        imageInfo.extent.height = static_cast<uint32_t>(extent.height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
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

    static VkSampler CreateSampler(VkDevice device, VkPhysicalDeviceProperties properties)
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        VkSampler sampler;
        VkResult result = vkCreateSampler(device, &samplerInfo, nullptr, &sampler);
        Assert(result == VK_SUCCESS);

        return sampler;
    }
}

Image::Image(ImageDescription aDescription, const VulkanContext* aVulkanContext)
    : vulkanContext{ aVulkanContext }
    , description{ aDescription }
{    
    using namespace ImageDetails;

    Assert(vulkanContext != nullptr);
    Assert(!description.filePath.empty());
    
    // For now we always load it here and so have to add this flag
    description.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    int texChannels;
    stbi_uc* pixels = stbi_load(description.filePath.c_str(), &extent.width, &extent.height, &texChannels, 
        STBI_rgb_alpha);
    Assert(pixels != nullptr);

    VkDeviceSize imageSize = extent.width * extent.height * 4;
    // TODO: find out why does this not work w/o cast :|
    std::span pixelsSpan = { const_cast<const stbi_uc*>(pixels), imageSize };

    // TODO: do not have it here?
    Buffer stagingBuffer(GetStagingBufferDescription(imageSize), false, pixelsSpan, vulkanContext);

    stbi_image_free(pixels);

    image = CreateImage(description, extent, *vulkanContext);

    TransitionImageLayout(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, *vulkanContext);

    CopyBufferToImage(stagingBuffer.GetVkBuffer(), image, extent, *vulkanContext);

    TransitionImageLayout(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, *vulkanContext);

    VkDevice device = vulkanContext->GetDevice().GetVkDevice();

    imageView = VulkanHelpers::CreateImageView(device, image, VK_FORMAT_R8G8B8A8_SRGB);
    sampler = CreateSampler(device, vulkanContext->GetDevice().GetPhysicalDeviceProperties());
}

Image::~Image()
{
    VkDevice device = vulkanContext->GetDevice().GetVkDevice();

    if (sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(device, sampler, nullptr);
    }    

    if (imageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(device, imageView, nullptr);
    }

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
    , imageView{ other.imageView }
    , sampler{ other.sampler }
    , extent{ other.extent }
{
    other.description = {};
    other.image = VK_NULL_HANDLE;    
    other.imageView = VK_NULL_HANDLE;
    other.sampler = VK_NULL_HANDLE;
    other.extent = {};
}

Image& Image::operator=(Image&& other) noexcept
{
    if (this != &other)
    {
        vulkanContext = other.vulkanContext;
        description = other.description;
        image = other.image;
        imageView = other.imageView;
        sampler = other.sampler;
        extent = other.extent;

        other.description = {};
        other.image = VK_NULL_HANDLE;
        other.imageView = VK_NULL_HANDLE;
        other.sampler = VK_NULL_HANDLE;
        other.extent = {};
    }
    return *this;
}