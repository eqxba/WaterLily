#pragma once

#include <volk.h>
#include <glm/glm.hpp>

#include "Engine/Render/Vulkan/Resources/Buffer.hpp"
#include "Engine/Render/Vulkan/Resources/Image.hpp"
#include "Engine/Render/Vulkan/Synchronization/Synchronization.hpp"

struct LayoutTransition
{
    VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
};

namespace LayoutTransitions
{
    constexpr LayoutTransition undefinedToGeneral = { VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL };
    constexpr LayoutTransition undefinedToDstOptimal = { VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL };
    constexpr LayoutTransition generalToSrcOptimal = { VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL };
    constexpr LayoutTransition dstOptimalToSrcOptimal = { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL };
    constexpr LayoutTransition dstOptimalToColorAttachmentOptimal = { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    constexpr LayoutTransition srcOptimalToShaderReadOnlyOptimal = { VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    constexpr LayoutTransition dstOptimalToShaderReadOnlyOptimal = { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
}

namespace ImageUtils
{
    // All mips
    void TransitionLayout(VkCommandBuffer commandBuffer, const Image& image, LayoutTransition transition, PipelineBarrier barrier = {});
    void TransitionLayout(VkCommandBuffer commandBuffer, const Image& image, LayoutTransition transition, PipelineBarrier barrier,
        uint32_t baseMipLevel, uint32_t mipLevelsCount = 1);

    void BlitImageToImage(VkCommandBuffer commandBuffer, const Image& source, const Image& destination);
    void BlitImageToImage(VkCommandBuffer commandBuffer, const Image& source, const Image& destination, VkOffset3D sourceOffset,
        VkOffset3D destinationOffset, uint32_t sourceMipLevel, uint32_t destinationMipLevel);

    // Only mip 0
    void CopyBufferToImage(const VkCommandBuffer commandBuffer, const Buffer& buffer, const Image& image);

    void GenerateMipMaps(const VkCommandBuffer commandBuffer, const Image& image);

    void FillImage(const VkCommandBuffer commandBuffer, const Image& image, const glm::vec4& color);
}
