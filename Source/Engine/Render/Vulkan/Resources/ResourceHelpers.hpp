#pragma once

#include "Engine/Render/Vulkan/Resources/Buffer.hpp"
#include "Utils/DataStructures.hpp"

namespace ResourceHelpers
{
    std::tuple<Buffer, Extent2D> LoadImageToBuffer(const std::string& absolutePath, const VulkanContext& vulkanContext);
}
