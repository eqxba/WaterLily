#include "Engine/Render/Vulkan/Image/ImageUtils.hpp"

namespace ImageUtilsDetails
{
    static inline VkOffset3D ToVkOffset3D(const VkExtent3D& extent)
    {
        return { static_cast<int32_t>(extent.width), static_cast<int32_t>(extent.height),
            static_cast<int32_t>(extent.depth) };
    }

    static VkExtent3D GetMipExtent(const Image& image, const uint32_t mipLevel)
    {
        Assert(mipLevel < image.GetDescription().mipLevelsCount);
        
        VkExtent3D extent = image.GetDescription().extent;
        
        for (uint32_t i = 0; i < mipLevel; ++i)
        {
            extent.width = extent.width > 1 ? extent.width / 2 : 1;
            extent.height = extent.height > 1 ? extent.height / 2 : 1;
            extent.depth = extent.depth > 1 ? extent.depth / 2 : 1;
        }
        
        return extent;
    }
}

void ImageUtils::TransitionLayout(const VkCommandBuffer commandBuffer, const Texture& texture,
    const LayoutTransition transition, const PipelineBarrier barrier)
{
    TransitionLayout(commandBuffer, texture, texture.view.GetAspectMask(), transition, barrier);
}

void ImageUtils::TransitionLayout(const VkCommandBuffer commandBuffer, const RenderTarget& renderTarget,
    const LayoutTransition transition, const PipelineBarrier barrier)
{
    TransitionLayout(commandBuffer, renderTarget, transition, barrier, 0, renderTarget.image.GetDescription().mipLevelsCount);
}

void ImageUtils::TransitionLayout(const VkCommandBuffer commandBuffer, const Image& image, const VkImageAspectFlags aspectMask,
    const LayoutTransition transition, const PipelineBarrier barrier)
{
    TransitionLayout(commandBuffer, image, aspectMask, transition, barrier, 0, image.GetDescription().mipLevelsCount);
}

void ImageUtils::TransitionLayout(VkCommandBuffer commandBuffer, const RenderTarget& renderTarget, LayoutTransition transition, PipelineBarrier barrier,
    const uint32_t baseMipLevel, const uint32_t mipLevelsCount /* = 1 */)
{
    TransitionLayout(commandBuffer, renderTarget, renderTarget.views[0].GetAspectMask(), transition, barrier, baseMipLevel, mipLevelsCount);
}

void ImageUtils::TransitionLayout(const VkCommandBuffer commandBuffer, const Image& image, const VkImageAspectFlags aspectMask,
    const LayoutTransition transition, const PipelineBarrier barrier, const uint32_t baseMipLevel,
    const uint32_t mipLevelsCount /* = 1 */)
{
    const VkImageSubresourceRange subresourceRange = {
        .aspectMask = aspectMask,
        .baseMipLevel = baseMipLevel,
        .levelCount = mipLevelsCount,
        .baseArrayLayer = 0,
        .layerCount = 1, };
    
    const VkImageMemoryBarrier imageMemoryBarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = barrier.srcAccessMask,
        .dstAccessMask = barrier.dstAccessMask,
        .oldLayout = transition.oldLayout,
        .newLayout = transition.newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = subresourceRange, };

    vkCmdPipelineBarrier(commandBuffer, barrier.srcStage, barrier.dstStage, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
}

void ImageUtils::BlitImageToImage(const VkCommandBuffer commandBuffer, const Image& source, const Image& destination,
    const uint32_t sourceMipLevel /* = 0 */, const uint32_t destinationMipLevel /* = 0 */, const VkFilter filter /* = VK_FILTER_LINEAR */)
{
    const VkExtent3D sourceExtent = ImageUtilsDetails::GetMipExtent(source, sourceMipLevel);
    const VkExtent3D destinationExtent = ImageUtilsDetails::GetMipExtent(destination, destinationMipLevel);

    const VkOffset3D sourceOffset = ImageUtilsDetails::ToVkOffset3D(sourceExtent);
    const VkOffset3D destinationOffset = ImageUtilsDetails::ToVkOffset3D(destinationExtent);
    
    BlitImageToImage(commandBuffer, source, destination, sourceOffset, destinationOffset, sourceMipLevel, destinationMipLevel, filter);
}

void ImageUtils::BlitImageToImage(const VkCommandBuffer commandBuffer, const Image& source, const Image& destination,
    const VkOffset3D sourceOffset, const VkOffset3D destinationOffset, const uint32_t sourceMipLevel,
    const uint32_t destinationMipLevel, const VkFilter filter /* = VK_FILTER_LINEAR */)
{
    const VkImageSubresourceLayers srcSubresource = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel = sourceMipLevel,
        .baseArrayLayer = 0,
        .layerCount = 1, };
    
    const VkImageSubresourceLayers dstSubresource = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel = destinationMipLevel,
        .baseArrayLayer = 0,
        .layerCount = 1, };
    
    const VkImageBlit blit = {
        .srcSubresource = srcSubresource,
        .srcOffsets = { { 0, 0, 0 }, sourceOffset },
        .dstSubresource = dstSubresource,
        .dstOffsets = { { 0, 0, 0 }, destinationOffset }, };

    vkCmdBlitImage(commandBuffer, source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destination,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, filter);
}

void ImageUtils::CopyBufferToImage(const VkCommandBuffer commandBuffer, const Buffer& buffer, const Image& image)
{
    // TODO: assert on size inequality?
    Assert((buffer.GetDescription().usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) == VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    Assert((image.GetDescription().usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) == VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    
    const VkImageSubresourceLayers imageSubresource = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel = 0,
        .baseArrayLayer = 0,
        .layerCount = 1, };
    
    const VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = imageSubresource,
        .imageOffset = { 0, 0, 0 },
        .imageExtent = image.GetDescription().extent, };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void ImageUtils::GenerateMipMaps(const VkCommandBuffer commandBuffer, const Texture& texture)
{
    using namespace ImageUtilsDetails;
    using namespace LayoutTransitions;
    
    const uint32_t mipLevelsCount = texture.image.GetDescription().mipLevelsCount;
    
    Assert(mipLevelsCount > 1);
    
    constexpr PipelineBarrier transferBarrier = {
        .srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT, .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT, .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT };

    const auto generateNextMipLevel = [&](uint32_t sourceMip) {
        TransitionLayout(commandBuffer, texture, texture.view.GetAspectMask(), dstOptimalToSrcOptimal, transferBarrier, sourceMip);
        BlitImageToImage(commandBuffer, texture, texture, ToVkOffset3D(GetMipExtent(texture, sourceMip)),
            ToVkOffset3D(GetMipExtent(texture, sourceMip + 1)), sourceMip, sourceMip + 1);
    };
    
    std::ranges::for_each(std::views::iota(static_cast<uint32_t>(0), texture.image.GetDescription().mipLevelsCount - 1),
        generateNextMipLevel);
    
    TransitionLayout(commandBuffer, texture, texture.view.GetAspectMask(), dstOptimalToSrcOptimal, transferBarrier, mipLevelsCount - 1);
}

void ImageUtils::FillImage(const VkCommandBuffer commandBuffer, const Image& image, const glm::vec4& color)
{
    const VkClearColorValue clearValue = { .float32 = { color.x, color.y, color.z, color.w } };
    
    const VkImageSubresourceRange clearRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1, };

    vkCmdClearColorImage(commandBuffer, image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);
}

uint32_t ImageUtils::MipLevelsCount(const VkExtent3D extent)
{
    const uint32_t maxDimension = std::max(std::max(extent.width, extent.height), extent.depth);
    return static_cast<uint32_t>(std::floor(std::log2(maxDimension))) + 1;
}
