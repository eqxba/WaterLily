#pragma once

#include "Engine/Render/Vulkan/Shaders/ShaderModule.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderCompiler.hpp"
#include "Engine/FileSystem/FileSystem.hpp"

class VulkanContext;

using ShaderDefine = std::pair<std::string_view, std::string_view>;

class ShaderManager
{
public:
    explicit ShaderManager(const VulkanContext& vulkanContext);

    ShaderModule CreateShaderModule(const FilePath& path, const ShaderType shaderType, const std::vector<ShaderDefine>& shaderDefines,
        bool useCacheOnFailure = true) const;

private:
    ShaderModule CreateShaderModule(std::span<const uint32_t> spirvCode, const ShaderType shaderType) const;
    
    const VulkanContext& vulkanContext;

    ShaderCompiler shaderCompiler;
};
