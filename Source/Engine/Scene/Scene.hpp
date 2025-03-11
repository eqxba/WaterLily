#pragma once

#include "Engine/FileSystem/FilePath.hpp"
#include "Engine/Scene/SceneDataStructures.hpp"
#include "Engine/Components/CameraComponent.hpp"
#include "Engine/Render/Vulkan/Buffer/Buffer.hpp"
#include "Engine/Render/Vulkan/Image/Texture.hpp"
#include "Engine/Render/Vulkan/DescriptorSets/DescriptorSetLayout.hpp"

#include <volk.h>

class VulkanContext;
class Image;

class Scene
{
public:
    static DescriptorSetLayout GetGlobalDescriptorSetLayout(const VulkanContext& vulkanContext);

    Scene(FilePath path, const VulkanContext& vulkanContext);
    ~Scene();

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    Scene(Scene&&) = delete;
    Scene& operator=(Scene&&) = delete;    

    const Buffer& GetVertexBuffer() const
    {
        return vertexBuffer;
    }

    const Buffer& GetIndexBuffer() const
    {
        return indexBuffer;
    }

    const Buffer& GetIndirectBuffer() const
    {
        return indirectBuffer;
    }

    const std::vector<VkDescriptorSet>& GetGlobalDescriptors() const
    {
        return globalDescriptors;
    }

    uint32_t GetDrawCount() const
    {
        return drawCount;
    }

    uint32_t GetIndirectDrawCount() const
    {
        return indirectDrawCount;
    }

    CameraComponent& GetCamera()
    {
        return camera;
    }

private:
    void InitFromGltfScene();

    void InitBuffers();
    void InitTexture();
    void InitGlobalDescriptors();

    const VulkanContext& vulkanContext;

    Buffer vertexBuffer;
    Buffer indexBuffer;
    Buffer transformBuffer;
    Buffer meshletDataBuffer;
    Buffer meshletBuffer;
    Buffer primitiveBuffer;
    Buffer drawBuffer;
    Buffer indirectBuffer;

    Texture texture;

    std::vector<VkDescriptorSet> globalDescriptors;
    DescriptorSetLayout globalDescriptorSetLayout;

    uint32_t drawCount = 0;
    uint32_t indirectDrawCount = 0;

    RawScene rawScene;

    CameraComponent camera = {};
    
    FilePath path;
};
