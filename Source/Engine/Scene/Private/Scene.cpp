#include "Engine/Scene/Scene.hpp"

#include "Engine/Scene/SceneHelpers.hpp"
#include "Engine/Render/Resources/StbImage.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Image/ImageUtils.hpp"

namespace SceneDetails
{
    static constexpr std::string_view imagePath = "~/Assets/texture.png";

    static uint64_t totalTriangles = 0;
}

uint64_t Scene::GetTotalTriangles()
{
    return SceneDetails::totalTriangles;
}

void Scene::SetTotalTriangles(uint64_t triangles)
{
    SceneDetails::totalTriangles = triangles;
}

Scene::Scene(FilePath aPath, const VulkanContext& aVulkanContext)
    : vulkanContext{ aVulkanContext }
    , path{ std::move(aPath) }
{
    if (std::optional<RawScene> loadResult = SceneHelpers::LoadGltfScene(path))
    {
        rawScene = std::move(loadResult.value());

        InitTexture();
    }
}

Scene::~Scene()
{}

void Scene::InitTexture()
{
    using namespace ImageUtils;
    
    const auto stbImage = StbImage(FilePath(SceneDetails::imagePath));
    const std::span<const std::byte> pixelData = std::as_bytes(stbImage.GetPixels());
    
    const BufferDescription stagingBufferDescription = { .size = pixelData.size(),
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
    
    const auto stagingBuffer = Buffer(stagingBufferDescription, false, pixelData, vulkanContext);

    const VkExtent3D extent = stbImage.GetExtent();
    const uint32_t mipLevelsCount = ImageUtils::MipLevelsCount(extent);

    ImageDescription textureDescription = {
        .extent = extent,
        .mipLevelsCount = mipLevelsCount,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, };

    SamplerDescription samplerDescription = {
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .maxAnisotropy = vulkanContext.GetDevice().GetProperties().physicalProperties.limits.maxSamplerAnisotropy,
        .maxLod = static_cast<float>(mipLevelsCount), };
    
    texture = Texture(std::move(textureDescription), std::move(samplerDescription), vulkanContext);
    
    vulkanContext.GetDevice().ExecuteOneTimeCommandBuffer([&](VkCommandBuffer commandBuffer) {
        TransitionLayout(commandBuffer, texture, LayoutTransitions::undefinedToDstOptimal, {
            .dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT, .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT });

        CopyBufferToImage(commandBuffer, stagingBuffer, texture);
        GenerateMipMaps(commandBuffer, texture);
        
        TransitionLayout(commandBuffer, texture, LayoutTransitions::srcOptimalToShaderReadOnlyOptimal, {
            .srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT, .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, .dstAccessMask = VK_ACCESS_SHADER_READ_BIT });
    });
}
