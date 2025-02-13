#include "Engine/Render/Vulkan/Resources/ResourceHelpers.hpp"

#include <stb_image.h>

std::tuple<Buffer, Extent2D> ResourceHelpers::LoadImageToBuffer(const FilePath& path, const VulkanContext& vulkanContext)
{
    Assert(path.Exists());

    Extent2D extent{};
    int texChannels;

    stbi_uc* pixels = stbi_load(path.GetAbsolute().c_str(), &extent.width, &extent.height, &texChannels, STBI_rgb_alpha);
    Assert(pixels != nullptr);

    const auto imageSize = static_cast<VkDeviceSize>(extent.width * extent.height * 4);
    // TODO: find out why does this not work w/o cast :|
    const std::span pixelsSpan = { const_cast<const stbi_uc*>(pixels), imageSize };

    const BufferDescription bufferDescription = { .size = imageSize,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

    Buffer buffer(bufferDescription, false, pixelsSpan, &vulkanContext);

    stbi_image_free(pixels);

    return { std::move(buffer), extent };
}
