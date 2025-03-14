#include "Engine/Scene/Scene.hpp"

#include "Utils/Helpers.hpp"
#include "Engine/Scene/SceneHelpers.hpp"
#include "Engine/Render/Resources/StbImage.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Image/ImageUtils.hpp"
#include "Engine/Render/Vulkan/Buffer/BufferUtils.hpp"

namespace SceneDetails
{
    static constexpr std::string_view imagePath = "~/Assets/texture.png";
}

DescriptorSetLayout Scene::GetGlobalDescriptorSetLayout(const VulkanContext& vulkanContext)
{
    // TODO: Parse from SPIR-V reflection
    return vulkanContext.GetDescriptorSetsManager().GetDescriptorSetLayoutBuilder()
        .AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT) 
        .AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT)
        .AddBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_MESH_BIT_EXT)
        .AddBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT)
        .AddBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT) // + fragment shader for materials
        .AddBinding(5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_TASK_BIT_EXT)
        .AddBinding(6, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT)
        .AddBinding(7, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .Build();
}

Scene::Scene(FilePath aPath, const VulkanContext& aVulkanContext)
    : vulkanContext{ aVulkanContext }
    , path{ std::move(aPath) }
{
    InitFromGltfScene();
}

Scene::~Scene()
{
    vulkanContext.GetDevice().WaitIdle();

    vulkanContext.GetDescriptorSetsManager().ResetDescriptors(DescriptorScope::eSceneRenderer);
}

void Scene::InitFromGltfScene()
{
    using namespace SceneHelpers;

    if (std::optional<RawScene> loadResult = LoadGltfScene(path); loadResult)
    {
        RawScene rawScene = std::move(loadResult.value());

        InitBuffers(rawScene);
        InitTexture();
        InitGlobalDescriptors();

        drawCount = static_cast<uint32_t>(rawScene.draws.size());
        indirectDrawCount = static_cast<uint32_t>(rawScene.indirectCommands.size());
    }
}

void Scene::InitBuffers(const RawScene& rawScene)
{
    using namespace BufferUtils;

    const std::span verticesSpan(std::as_const(rawScene.vertices));

    BufferDescription vertexBufferDescription{
        .size = verticesSpan.size_bytes(),
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

    vertexBuffer = Buffer(std::move(vertexBufferDescription), true, verticesSpan, vulkanContext);

    const std::span indicesSpan(std::as_const(rawScene.indices));

    BufferDescription indexBufferDescription{
        .size = indicesSpan.size_bytes(),
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

    indexBuffer = Buffer(std::move(indexBufferDescription), true, indicesSpan, vulkanContext);

    const std::span transformsSpan(std::as_const(rawScene.transforms));

    BufferDescription transformsBufferDescription{
        .size = transformsSpan.size_bytes(),
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

    transformBuffer = Buffer(std::move(transformsBufferDescription), true, transformsSpan, vulkanContext);

    const std::span meshletDataSpan(std::as_const(rawScene.meshletData));

    BufferDescription meshletDataBufferDescription{
        .size = meshletDataSpan.size_bytes(),
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

    meshletDataBuffer = Buffer(std::move(meshletDataBufferDescription), true, meshletDataSpan, vulkanContext);

    const std::span meshletsSpan(std::as_const(rawScene.meshlets));

    BufferDescription meshletBufferDescription{
        .size = meshletsSpan.size_bytes(),
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

    meshletBuffer = Buffer(std::move(meshletBufferDescription), true, meshletsSpan, vulkanContext);

    const std::span primitiveSpan(std::as_const(rawScene.gpuPrimitives));

    BufferDescription primitiveBufferDescription{
        .size = primitiveSpan.size_bytes(),
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

    primitiveBuffer = Buffer(std::move(primitiveBufferDescription), true, primitiveSpan, vulkanContext);

    const std::span drawsSpan(std::as_const(rawScene.draws));

    BufferDescription drawsBufferDescription{
        .size = drawsSpan.size_bytes(),
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

    drawBuffer = Buffer(std::move(drawsBufferDescription), true, drawsSpan, vulkanContext);

    std::span indirectCommandsSpan(std::as_const(rawScene.indirectCommands));

    BufferDescription indirectBufferDescription = {
        .size = indirectCommandsSpan.size_bytes(),
        .usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

    indirectBuffer = Buffer(std::move(indirectBufferDescription), true, indirectCommandsSpan, vulkanContext);

    PipelineBarrier barrier = {
        .srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstStage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
        .dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT };
    
    const Device& device = vulkanContext.GetDevice();
    
    if (device.GetProperties().meshShadersSupported)
    {
        barrier.dstStage |= VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT;
        barrier.dstAccessMask |= VK_ACCESS_SHADER_READ_BIT;
    }
    
    device.ExecuteOneTimeCommandBuffer([&](const VkCommandBuffer commandBuffer) {
        CopyBufferToBuffer(commandBuffer, vertexBuffer.GetStagingBuffer(), vertexBuffer);
        CopyBufferToBuffer(commandBuffer, indexBuffer.GetStagingBuffer(), indexBuffer);
        CopyBufferToBuffer(commandBuffer, transformBuffer.GetStagingBuffer(), transformBuffer);
        CopyBufferToBuffer(commandBuffer, meshletDataBuffer.GetStagingBuffer(), meshletDataBuffer);
        CopyBufferToBuffer(commandBuffer, meshletBuffer.GetStagingBuffer(), meshletBuffer);
        CopyBufferToBuffer(commandBuffer, primitiveBuffer.GetStagingBuffer(), primitiveBuffer);
        CopyBufferToBuffer(commandBuffer, drawBuffer.GetStagingBuffer(), drawBuffer);
        CopyBufferToBuffer(commandBuffer, indirectBuffer.GetStagingBuffer(), indirectBuffer);
        
        SynchronizationUtils::SetMemoryBarrier(commandBuffer, barrier);
    });

    vertexBuffer.DestroyStagingBuffer();
    indexBuffer.DestroyStagingBuffer();
    transformBuffer.DestroyStagingBuffer();
    meshletDataBuffer.DestroyStagingBuffer();
    meshletBuffer.DestroyStagingBuffer();
    primitiveBuffer.DestroyStagingBuffer();
    drawBuffer.DestroyStagingBuffer();
    indirectBuffer.DestroyStagingBuffer();
}

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
    const uint32_t maxDimension = std::max(extent.width, extent.height);
    const uint32_t mipLevelsCount = static_cast<uint32_t>(std::floor(std::log2(maxDimension))) + 1;

    ImageDescription textureDescription = {
        .extent = extent,
        .mipLevelsCount = mipLevelsCount,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, };

    SamplerDescription samplerDescription = {
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

void Scene::InitGlobalDescriptors()
{
    globalDescriptorSetLayout = GetGlobalDescriptorSetLayout(vulkanContext);

    const auto [descriptor, layout] = vulkanContext.GetDescriptorSetsManager().GetDescriptorSetBuilder(globalDescriptorSetLayout)
        .Bind(0, vertexBuffer)
        .Bind(1, transformBuffer)
        .Bind(2, meshletDataBuffer)
        .Bind(3, meshletBuffer)
        .Bind(4, primitiveBuffer)
        .Bind(5, drawBuffer)
        .Bind(6, texture.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        .Bind(7, texture.sampler)
        .Build();

    globalDescriptors.push_back(descriptor);
}
