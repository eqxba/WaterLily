#pragma once

#include "Engine/Render/Vulkan/Resources/Buffer.hpp"
#include "Engine/Render/Vulkan/Resources/Image.hpp"
#include "Engine/Render/Vulkan/Resources/ImageView.hpp"
#include "Engine/Components/CameraComponent.hpp"

#include <glm/glm.hpp>
#include <volk.h>

class VulkanContext;
class Image;

struct Vertex
{
    static VkVertexInputBindingDescription GetBindingDescription();
    static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();

    bool operator==(const Vertex& other) const;

    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
};

class Scene
{
public:
    Scene(const VulkanContext& vulkanContext);
    ~Scene();

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    Scene(Scene&&) = delete;
    Scene& operator=(Scene&&) = delete;    

    const Buffer& GetVertexBuffer() const
    {
        return vertexBuffer;
    }

    const Buffer& GetIndexBuffer() const
    {
        return indexBuffer;
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
    void InitBuffers();
    void InitTextureResources();

    const VulkanContext& vulkanContext;

    Buffer vertexBuffer;
    Buffer indexBuffer;

    Image image;
    ImageView imageView;
    VkSampler sampler = VK_NULL_HANDLE; // TODO: move away to a proper location, pool or smth

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    CameraComponent camera = {};
};