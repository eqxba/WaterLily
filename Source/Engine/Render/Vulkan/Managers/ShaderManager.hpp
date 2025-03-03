#pragma once

#include "Engine/Render/Vulkan/Resources/Shaders/ShaderModule.hpp"
#include "Engine/Render/Vulkan/Resources/Shaders/ShaderCompiler.hpp"
#include "Engine/FileSystem/FileSystem.hpp"

class VulkanContext;

class ShaderManager
{
public:
    explicit ShaderManager(const VulkanContext& vulkanContext);

    ShaderModule CreateShaderModule(const FilePath& path, const ShaderType shaderType, bool useCacheOnFailure = true) const;

private:
    ShaderModule CreateShaderModule(std::span<const uint32_t> spirvCode, const ShaderType shaderType) const;
    
    const VulkanContext& vulkanContext;

    ShaderCompiler shaderCompiler;
};
