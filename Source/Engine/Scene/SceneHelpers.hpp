#pragma once

#include "Engine/Scene/SceneDataStructures.hpp"
#include "Engine/FileSystem/FilePath.hpp"

namespace tinygltf
{
    class Model;
    class Node;
}

struct SceneNode;

namespace SceneHelpers
{
    std::optional<RawScene> LoadGltfScene(const FilePath& path);

    std::vector<VkVertexInputBindingDescription> GetVertexBindings();
    std::vector<VkVertexInputAttributeDescription> GetVertexAttributes();
}
