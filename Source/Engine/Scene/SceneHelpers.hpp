#pragma once

#include "Engine/Scene/SceneDataStructures.hpp"

namespace tinygltf
{
    class Model;
    class Node;
}

struct SceneNode;

namespace SceneHelpers
{
    std::tuple<std::vector<Vertex>, std::vector<uint32_t>> LoadObjModel(const std::string& absolutePath, 
        bool flipYz = false);

    std::unique_ptr<tinygltf::Model> LoadGltfScene(const std::string& absolutePath);

    std::unique_ptr<SceneNode> LoadGltfHierarchy(const tinygltf::Node& node, const tinygltf::Model& model, 
        std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);
}
