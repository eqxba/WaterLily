#pragma once

#include "Engine/Scene/SceneDataStructures.hpp"
#include "Engine/FileSystem/FilePath.hpp"

namespace SceneHelpers
{
    std::optional<RawScene> LoadGltfScene(const FilePath& path);

    std::vector<VkVertexInputBindingDescription> GetVertexBindings();
    std::vector<VkVertexInputAttributeDescription> GetVertexAttributes();
}
