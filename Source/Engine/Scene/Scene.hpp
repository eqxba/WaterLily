#pragma once

#include "Engine/FileSystem/FilePath.hpp"
#include "Engine/Scene/SceneDataStructures.hpp"
#include "Engine/Components/CameraComponent.hpp"
#include "Engine/Render/Vulkan/Buffer/Buffer.hpp"
#include "Engine/Render/Vulkan/Image/Texture.hpp"

#include <volk.h>

class VulkanContext;
class Image;

class Scene
{
public:
    Scene(FilePath path, const VulkanContext& vulkanContext);
    ~Scene();

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    Scene(Scene&&) = delete;
    Scene& operator=(Scene&&) = delete;

    CameraComponent& GetCamera()
    {
        return camera;
    }
    
    Buffer vertexBuffer;
    Buffer indexBuffer;
    Buffer transformBuffer;
    Buffer meshletDataBuffer;
    Buffer meshletBuffer;
    Buffer primitiveBuffer;
    Buffer drawBuffer;
    Buffer indirectBuffer;
    
    uint32_t drawCount = 0;
    uint32_t indirectDrawCount = 0;

private:
    void InitFromGltfScene();

    void InitBuffers(const RawScene& rawScene);
    void InitTexture();

    const VulkanContext& vulkanContext;

    Texture texture;

    CameraComponent camera = {};
    
    FilePath path;
};
