#include "Engine/Scene/Scene.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

VkVertexInputBindingDescription Vertex::GetBindingDescription()
{
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription> Vertex::GetAttributeDescriptions()
{
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);

    // TODO: (low priority) parse from compiled shader file
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    return attributeDescriptions;
}

Scene::Scene(const VulkanContext& aVulkanContext)
    : vulkanContext{aVulkanContext}
{
    std::span verticesSpan(std::as_const(vertices));
    std::span indicesSpan(std::as_const(indices));

    BufferDescription vertexBufferDescription{ static_cast<VkDeviceSize>(verticesSpan.size_bytes()),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true };

    vertexBuffer = std::make_unique<Buffer>(vertexBufferDescription, vulkanContext, verticesSpan);

    BufferDescription indexBufferDescription{ static_cast<VkDeviceSize>(indicesSpan.size_bytes()),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true };

    indexBuffer = std::make_unique<Buffer>(indexBufferDescription, vulkanContext, indicesSpan);
}
