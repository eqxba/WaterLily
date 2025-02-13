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
    std::tuple<std::vector<Vertex>, std::vector<uint32_t>> LoadObjModel(const FilePath& path, bool flipYz = false);

    std::unique_ptr<tinygltf::Model> LoadGltfScene(const FilePath& path);

    std::unique_ptr<SceneNode> LoadGltfHierarchy(const tinygltf::Node& node, const tinygltf::Model& model, 
        std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);

    std::vector<SceneNode*> GetFlattenNodes(SceneNode& node);
    std::vector<glm::mat4> GetBakedTransforms(const SceneNode& node);

    void AssignNodeIdsToPrimitives(const std::vector<SceneNode*>& nodes);
}
