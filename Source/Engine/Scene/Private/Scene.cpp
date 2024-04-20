#include "Engine/Scene/Scene.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceHelpers.hpp"
#include "macros.hpp"

DISABLE_WARNINGS_BEGIN

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

DISABLE_WARNINGS_END

namespace std {
    template<>
    struct hash<Vertex>
    {
        size_t operator()(Vertex const& vertex) const
        {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

namespace SceneDetails
{
    constexpr const char* objPathWin = "E:/Projects/WaterLily/Assets/model.obj";
    constexpr const char* imagePathWin = "E:/Projects/WaterLily/Assets/texture.png";

    constexpr const char* objPathMac = "/Users/barboss/Projects/WaterLily/Assets/model.obj";
    constexpr const char* imagePathMac = "/Users/barboss/Projects/WaterLily/Assets/texture.png";

    constexpr const char* objPath = platformWin ? objPathWin : objPathMac;
    constexpr const char* imagePath = platformWin ? imagePathWin : imagePathMac;

    static std::tuple<std::vector<Vertex>, std::vector<uint32_t>> LoadModel(const std::string& absolutePath, 
        const bool flipYz = false)
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        tinyobj::attrib_t attributes;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;

        std::string warn, err;

        if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warn, &err, absolutePath.c_str()))
        {
            Assert(false);
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        const auto processMeshIndex = [&](const tinyobj::index_t& index) {
            Vertex vertex{};

            vertex.pos = {
                attributes.vertices[3 * index.vertex_index + 0],
                attributes.vertices[3 * index.vertex_index + 1],
                attributes.vertices[3 * index.vertex_index + 2], };

            if (flipYz)
            {
                std::swap(vertex.pos.y, vertex.pos.z);
                vertex.pos.z *= -1;
            }

            vertex.texCoord = {
                attributes.texcoords[2 * index.texcoord_index + 0],
                1.0f - attributes.texcoords[2 * index.texcoord_index + 1], };

            vertex.color = { 1.0f, 1.0f, 1.0f };

            if (uniqueVertices.count(vertex) == 0)
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        };

        std::ranges::for_each(shapes, [&](const auto& shape) {
            std::ranges::for_each(shape.mesh.indices, processMeshIndex);
        });

        return { std::move(vertices), std::move(indices) };
    }
}

bool Vertex::operator==(const Vertex& other) const 
{
    return pos == other.pos && color == other.color && texCoord == other.texCoord;
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
    : vulkanContext{ aVulkanContext }
{
    InitBuffers();
    InitTextureResources();
}

Scene::~Scene()
{
    VkDevice device = vulkanContext.GetDevice().GetVkDevice();
    
    VulkanHelpers::DestroySampler(device, sampler);
}

void Scene::InitBuffers()
{
    std::tie(vertices, indices) = SceneDetails::LoadModel(SceneDetails::objPath, true);

    std::span verticesSpan(std::as_const(vertices));
    std::span indicesSpan(std::as_const(indices));

    BufferDescription vertexBufferDescription{ .size = static_cast<VkDeviceSize>(verticesSpan.size_bytes()),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

    vertexBuffer = Buffer(vertexBufferDescription, true, verticesSpan, &vulkanContext);

    BufferDescription indexBufferDescription{ .size = static_cast<VkDeviceSize>(indicesSpan.size_bytes()),
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

    indexBuffer = Buffer(indexBufferDescription, true, indicesSpan, &vulkanContext);
}

void Scene::InitTextureResources()
{
    const Device& device = vulkanContext.GetDevice();

    const auto& [buffer, extent] = ResourceHelpers::LoadImageToBuffer(SceneDetails::imagePath, vulkanContext);

    const int maxDimension = std::max(extent.width, extent.height);
    const uint32_t mipLevelsCount = static_cast<uint32_t>(std::floor(std::log2(maxDimension))) + 1;

    ImageDescription imageDescription{ .extent = extent, .mipLevelsCount = mipLevelsCount,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT ,
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

    image = Image(imageDescription, &vulkanContext);
    image.FillMipLevel0(buffer, true);

    imageView = ImageView(image, VK_IMAGE_ASPECT_COLOR_BIT, &vulkanContext);

    sampler = VulkanHelpers::CreateSampler(device.GetVkDevice(), device.GetPhysicalDeviceProperties(), mipLevelsCount);
}
