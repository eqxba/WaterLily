#include "Engine/Scene/SceneDataStructures.hpp"

bool Vertex::operator==(const Vertex& other) const
{
    return pos == other.pos && normal == other.normal && uv == other.uv && color == other.color && 
        tangent == other.tangent;
}

std::vector<VkVertexInputBindingDescription> Vertex::GetBindings()
{
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(Vertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return { binding };
}

// TODO: (low priority) parse from compiled shader file
std::vector<VkVertexInputAttributeDescription> Vertex::GetAttributes()
{
    std::vector<VkVertexInputAttributeDescription> attributes(5);

    VkVertexInputAttributeDescription& posAttribute = attributes[0];
    posAttribute.binding = 0;
    posAttribute.location = 0;
    posAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    posAttribute.offset = offsetof(Vertex, pos);

    VkVertexInputAttributeDescription& normalAttribute = attributes[1];
    normalAttribute.binding = 0;
    normalAttribute.location = 1;
    normalAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    normalAttribute.offset = offsetof(Vertex, normal);

    VkVertexInputAttributeDescription& uvAttribute = attributes[2];
    uvAttribute.binding = 0;
    uvAttribute.location = 2;
    uvAttribute.format = VK_FORMAT_R32G32_SFLOAT;
    uvAttribute.offset = offsetof(Vertex, uv);

    VkVertexInputAttributeDescription& colorAttribute = attributes[3];
    colorAttribute.binding = 0;
    colorAttribute.location = 3;
    colorAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    colorAttribute.offset = offsetof(Vertex, color);

    VkVertexInputAttributeDescription& tangentAttribute = attributes[4];
    tangentAttribute.binding = 0;
    tangentAttribute.location = 4;
    tangentAttribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    tangentAttribute.offset = offsetof(Vertex, tangent);

    return attributes;
}