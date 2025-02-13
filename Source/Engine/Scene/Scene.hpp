#pragma once

#include "Engine/Scene/SceneDataStructures.hpp"
#include "Engine/Components/CameraComponent.hpp"
#include "Engine/Render/Vulkan/Resources/Buffer.hpp"
#include "Engine/Render/Vulkan/Resources/Image.hpp"
#include "Engine/Render/Vulkan/Resources/ImageView.hpp"
#include "Engine/FileSystem/FilePath.hpp"

#include <volk.h>

class VulkanContext;
class Image;

class Scene
{
public:
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

    const std::vector<Vertex>& GetVertices() const
    {
        return vertices;
    }

    const std::vector<uint32_t>& GetIndices() const
    {
        return indices;
    }

    const Image& GetImage() const
    {
        return image;
    }

    const ImageView& GetImageView() const
    {
        return imageView;
    }

    VkSampler GetSampler() const
    {
        return sampler;
    }

    CameraComponent& GetCamera()
    {
        return camera;
    }

private:
    void InitFromGltfScene();

    void InitBuffers();
    void InitTextureResources();

    const VulkanContext& vulkanContext;

    std::unique_ptr<SceneNode> root;
    std::vector<SceneNode*> nodes;

    Buffer vertexBuffer;
    Buffer indexBuffer;

    Buffer transformsBuffer;

    Image image;
    ImageView imageView;
    VkSampler sampler = VK_NULL_HANDLE; // TODO: move away to a proper location, pool or smth

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    CameraComponent camera = {};
    
    FilePath path;
};
