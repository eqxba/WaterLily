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

    std::vector<Vertex> vertices = 
    {
        {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
    };

    VkBuffer vertexBuffer;
};