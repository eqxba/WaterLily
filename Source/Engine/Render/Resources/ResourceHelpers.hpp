#pragma once

#include "Engine/Render/Resources/Buffer.hpp"
#include "Engine/FileSystem/FileSystem.hpp"
#include "Utils/DataStructures.hpp"

struct ImageLayoutTransition
{
    VkImageLayout srcLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout dstLayout = VK_IMAGE_LAYOUT_UNDEFINED;
};

namespace ImageLayoutTransitions
{
    constexpr ImageLayoutTransition undefinedToDstOptimal = { VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL };
    constexpr ImageLayoutTransition dstOptimalToSrcOptimal = { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL };
    constexpr ImageLayoutTransition srcOptimalToShaderReadOnlyOptimal = { VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    constexpr ImageLayoutTransition dstOptimalToShaderReadOnlyOptimal = { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
}

namespace ResourceHelpers
{
    std::tuple<Buffer, Extent2D> LoadImageToBuffer(const FilePath& path, const VulkanContext& vulkanContext);
}
