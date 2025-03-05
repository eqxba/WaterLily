#pragma once

#include "Engine/FileSystem/FilePath.hpp"
#include "Engine/Scene/SceneDataStructures.hpp"
#include "Engine/Components/CameraComponent.hpp"
#include "Engine/Render/Vulkan/Buffer/Buffer.hpp"
#include "Engine/Render/Vulkan/Image/Texture.hpp"
#include "Engine/Render/Vulkan/DescriptorSets/DescriptorSetLayout.hpp"

#include <volk.h>

class VulkanContext;
class Image;

class Scene
{
public:
    static DescriptorSetLayout GetGlobalDescriptorSetLayout(const VulkanContext& vulkanContext);

    Scene(FilePath path, const VulkanContext& vulkanContext);
    ~Scene();

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    Scene(Scene&&) = delete;
    Scene& operator=(Scene&&) = delete;    

    const SceneNode& GetRoot() const
    {
        return *root;
    }

    const std::vector<SceneNode*>& GetNodes() const
    {
        return nodes;
    }

    const Buffer& GetVertexBuffer() const
    {
        return vertexBuffer;
    }

    const Buffer& GetIndexBuffer() const
    {
        return indexBuffer;
    }

    const Buffer& GetTransformsBuffer() const
    {
        return transformsBuffer;
    }

    const std::vector<VkDescriptorSet>& GetGlobalDescriptors() const
    {
        return globalDescriptors;
    }

    const std::vector<Vertex>& GetVertices() const
    {
        return vertices;
    }

    const std::vector<uint32_t>& GetIndices() const
    {
        return indices;
    }

    const Texture& GetTexture() const
    {
        return texture;
    }

    CameraComponent& GetCamera()
    {
        return camera;
    }

private:
    void InitFromGltfScene();

    void InitBuffers();
    void InitTexture();
    void InitGlobalDescriptors();

    const VulkanContext& vulkanContext;

    std::unique_ptr<SceneNode> root;
    std::vector<SceneNode*> nodes;

    Buffer vertexBuffer;
    Buffer indexBuffer;

    Buffer transformsBuffer;

    Texture texture;

    std::vector<VkDescriptorSet> globalDescriptors;
    DescriptorSetLayout globalDescriptorSetLayout;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    CameraComponent camera = {};
    
    FilePath path;
};
