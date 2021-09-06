#pragma once

#include <glm/glm.hpp>
#include <volk.h>

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
    Scene();
    ~Scene();

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

    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;
};