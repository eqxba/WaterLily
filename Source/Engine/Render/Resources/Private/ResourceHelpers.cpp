#include "Engine/Render/Resources/ResourceHelpers.hpp"

#include <stb_image.h>

std::tuple<Buffer, Extent2D> ResourceHelpers::LoadImageToBuffer(const FilePath& path, const VulkanContext& vulkanContext)
{
    Assert(path.Exists());

    Extent2D extent{};
    int texChannels;

    stbi_uc* pixels = stbi_load(path.GetAbsolute().c_str(), &extent.width, &extent.height, &texChannels, STBI_rgb_alpha);
    Assert(pixels != nullptr);

    const auto imageSize = static_cast<size_t>(extent.width) * extent.height * 4;
    const std::span<const stbi_uc> pixelsSpan = { pixels, imageSize };

    const BufferDescription bufferDescription = { .size = imageSize,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

    Buffer buffer(bufferDescription, false, pixelsSpan, vulkanContext);

    stbi_image_free(pixels);

    return { std::move(buffer), extent };
}
