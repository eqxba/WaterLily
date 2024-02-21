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
    BufferDescription vertexBufferDescription{};
    vertexBufferDescription.size = static_cast<VkDeviceSize>(sizeof(vertices[0]) * vertices.size());
    vertexBufferDescription.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vertexBufferDescription.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    BufferManager& bufferManager = vulkanContext.GetBufferManager();

    vertexBuffer = bufferManager.CreateBuffer(vertexBufferDescription);
    bufferManager.FillBuffer(vertexBuffer, static_cast<void*>(vertices.data()), vertices.size() * sizeof(vertices[0]));

    BufferDescription indexBufferDescription{};
    indexBufferDescription.size = static_cast<VkDeviceSize>(sizeof(indices[0]) * indices.size());
    indexBufferDescription.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    indexBufferDescription.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    indexBuffer = bufferManager.CreateBuffer(indexBufferDescription);
    bufferManager.FillBuffer(indexBuffer, static_cast<void*>(indices.data()), indices.size() * sizeof(indices[0]));
}

Scene::~Scene()
{
    BufferManager& bufferManager = vulkanContext.GetBufferManager();

    bufferManager.DestroyBuffer(vertexBuffer);
    bufferManager.DestroyBuffer(indexBuffer);
}
