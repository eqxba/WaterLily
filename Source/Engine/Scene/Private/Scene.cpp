#include "Engine/Scene/Scene.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceHelpers.hpp"

namespace SceneDetails
{
    constexpr const char* imagePath = "E:/Projects/WaterLily/Assets/testImage.jpg";
}

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
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

    // TODO: (low priority) parse from compiled shader file
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

    return attributeDescriptions;
}

Scene::Scene(const VulkanContext& aVulkanContext)
    : vulkanContext{aVulkanContext}
{
    std::span verticesSpan(std::as_const(vertices));
    std::span indicesSpan(std::as_const(indices));

    BufferDescription vertexBufferDescription{ .size = static_cast<VkDeviceSize>(verticesSpan.size_bytes()),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

    vertexBuffer = std::make_unique<Buffer>(vertexBufferDescription, true, verticesSpan, &vulkanContext);

    BufferDescription indexBufferDescription{ .size = static_cast<VkDeviceSize>(indicesSpan.size_bytes()),
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

    indexBuffer = std::make_unique<Buffer>(indexBufferDescription, true, indicesSpan, &vulkanContext);

    const auto& [buffer, extent] = ResourceHelpers::LoadImageToBuffer(SceneDetails::imagePath, vulkanContext);

    ImageDescription imageDescription{ .extent = extent, .format = VK_FORMAT_R8G8B8A8_SRGB,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

    image = std::make_unique<Image>(imageDescription, &vulkanContext);

    image->Fill(buffer);
    
    const Device& device = vulkanContext.GetDevice();

    imageView = image->CreateImageView(VK_IMAGE_ASPECT_COLOR_BIT);
    sampler = VulkanHelpers::CreateSampler(device.GetVkDevice(), device.GetPhysicalDeviceProperties());
}

Scene::~Scene()
{
    VkDevice device = vulkanContext.GetDevice().GetVkDevice();
    
    VulkanHelpers::DestroySampler(device, sampler);
    VulkanHelpers::DestroyImageView(device, imageView);
}