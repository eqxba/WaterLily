#include "Engine/Scene/Scene.hpp"

#include "Engine/Scene/SceneHelpers.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceHelpers.hpp"
#include "Utils/Helpers.hpp"

DISABLE_WARNINGS_BEGIN
#include <tiny_gltf.h>
DISABLE_WARNINGS_END

namespace SceneDetails
{
    constexpr const char* scenePathWin = "E:/Projects/WaterLily/Assets/Scenes/Sponza/glTF/Sponza.gltf";
    constexpr const char* imagePathWin = "E:/Projects/WaterLily/Assets/texture.png";

    constexpr const char* scenePathMac = "/Users/barboss/Projects/WaterLily/Assets/Scenes/Sponza/glTF/Sponza.gltf";
    constexpr const char* imagePathMac = "/Users/barboss/Projects/WaterLily/Assets/texture.png";

    constexpr const char* scenePath = platformWin ? scenePathWin : scenePathMac;
    constexpr const char* imagePath = platformWin ? imagePathWin : imagePathMac;
}

Scene::Scene(const VulkanContext& aVulkanContext)
    : vulkanContext{ aVulkanContext }
    , root{ std::make_unique<SceneNode>() }
{
    InitFromGltfScene();
    InitBuffers();
    InitTextureResources();
}

Scene::~Scene()
{
    VkDevice device = vulkanContext.GetDevice().GetVkDevice();
    
    VulkanHelpers::DestroySampler(device, sampler);
}

void Scene::InitFromGltfScene()
{
    using namespace SceneHelpers;

    std::unique_ptr<tinygltf::Model> gltfModel = LoadGltfScene(SceneDetails::scenePath);

    {
        ScopeTimer timer("Convert gltf scene");

        for (size_t i = 0; i < gltfModel->scenes[0].nodes.size(); i++)
        {
            root->children.emplace_back(LoadGltfHierarchy(gltfModel->nodes[i], *gltfModel, indices, vertices));
        }
    }
}

void Scene::InitBuffers()
{
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
