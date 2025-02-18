#pragma once

#include "Engine/Render/Resources/Shaders/ShaderModule.hpp"
#include "Engine/Render/Resources/Shaders/ShaderCompiler.hpp"
#include "Engine/FileSystem/FileSystem.hpp"

class VulkanContext;

class ShaderManager
{
public:
    explicit ShaderManager(const VulkanContext& vulkanContext);

    ShaderModule CreateShaderModule(const FilePath& path, const ShaderType shaderType) const;

private:
    const VulkanContext& vulkanContext;

    ShaderCompiler shaderCompiler;
};
