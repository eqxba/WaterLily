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

    const RawScene& GetRaw()
    {
        return rawScene;
    }

private:
    void InitTexture();

    const VulkanContext& vulkanContext;

    Texture texture;

    CameraComponent camera = {};
    
    FilePath path;

    RawScene rawScene;
};
