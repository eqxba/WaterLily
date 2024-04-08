#include "Engine/Render/Vulkan/Resources/ResourceHelpers.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

std::tuple<Buffer, Extent2D> ResourceHelpers::LoadImageToBuffer(const std::string& absolutePath, const VulkanContext& vulkanContext)
{
    Assert(!absolutePath.empty());

    Extent2D extent{};
    int texChannels;

    stbi_uc* pixels = stbi_load(absolutePath.c_str(), &extent.width, &extent.height, &texChannels, STBI_rgb_alpha);
    Assert(pixels != nullptr);

    VkDeviceSize imageSize = extent.width * extent.height * 4;
    // TODO: find out why does this not work w/o cast :|
    std::span pixelsSpan = { const_cast<const stbi_uc*>(pixels), imageSize };

    BufferDescription bufferDescription = { .size = imageSize,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

    Buffer buffer(bufferDescription, false, pixelsSpan, &vulkanContext);

    stbi_image_free(pixels);

    return { std::move(buffer), extent };
}