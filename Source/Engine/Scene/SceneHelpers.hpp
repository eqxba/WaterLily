#pragma once

#include "Engine/Scene/SceneDataStructures.hpp"
#include "Engine/FileSystem/FilePath.hpp"

namespace SceneHelpers
{
    std::optional<RawScene> LoadGltfScene(const FilePath& path);

    // TODO: Actually get this from scene traversal
    std::vector<gpu::Draw> GenerateDraws(const RawScene& rawScene);

    std::vector<VkVertexInputBindingDescription> GetVertexBindings();
    std::vector<VkVertexInputAttributeDescription> GetVertexAttributes();
}
