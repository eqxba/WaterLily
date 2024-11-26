#include "Engine/Scene/SceneDataStructures.hpp"

bool Vertex::operator==(const Vertex& other) const
{
    return pos == other.pos && normal == other.normal && uv == other.uv && color == other.color && 
        tangent == other.tangent;
}

VkVertexInputBindingDescription Vertex::GetBindingDescription()
{
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

// TODO: (low priority) parse from compiled shader file
std::vector<VkVertexInputAttributeDescription> Vertex::GetAttributeDescriptions()
{
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(5);

    VkVertexInputAttributeDescription& posAttribute = attributeDescriptions[0];
    posAttribute.binding = 0;
    posAttribute.location = 0;
    posAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    posAttribute.offset = offsetof(Vertex, pos);

    VkVertexInputAttributeDescription& normalAttribute = attributeDescriptions[1];
    normalAttribute.binding = 0;
    normalAttribute.location = 1;
    normalAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    normalAttribute.offset = offsetof(Vertex, normal);

    VkVertexInputAttributeDescription& uvAttribute = attributeDescriptions[2];
    uvAttribute.binding = 0;
    uvAttribute.location = 2;
    uvAttribute.format = VK_FORMAT_R32G32_SFLOAT;
    uvAttribute.offset = offsetof(Vertex, uv);

    VkVertexInputAttributeDescription& colorAttribute = attributeDescriptions[3];
    colorAttribute.binding = 0;
    colorAttribute.location = 3;
    colorAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    colorAttribute.offset = offsetof(Vertex, color);

    VkVertexInputAttributeDescription& tangentAttribute = attributeDescriptions[4];
    tangentAttribute.binding = 0;
    tangentAttribute.location = 4;
    tangentAttribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    tangentAttribute.offset = offsetof(Vertex, tangent);

    return attributeDescriptions;
}