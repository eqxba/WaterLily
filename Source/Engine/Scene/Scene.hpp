#pragma once

#include "Engine/Render/Vulkan/Resources/Buffer.hpp"

#include <glm/glm.hpp>
#include <volk.h>

class VulkanContext;

struct Vertex
{
    static VkVertexInputBindingDescription GetBindingDescription();
    static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();

    glm::vec2 pos;
    glm::vec3 color;
};

class Scene
{
public:
    Scene(const VulkanContext& vulkanContext);
    ~Scene() = default;

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    Scene(Scene&&) = delete;
    Scene& operator=(Scene&&) = delete;    

    const Buffer* GetVertexBuffer() const
    {
        return vertexBuffer.get();
    }

    const Buffer* GetIndexBuffer() const
    {
        return indexBuffer.get();
    }

    const std::vector<Vertex>& GetVertices()
    {
        return vertices;
    }

    const std::vector<uint16_t>& GetIndices()
    {
        return indices;
    }

private:
    const VulkanContext& vulkanContext;    

    std::unique_ptr<Buffer> vertexBuffer;
    std::unique_ptr<Buffer> indexBuffer;

    std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
    };

    std::vector<uint16_t> indices =
    {
        0, 1, 2, 2, 3, 0
    };
};