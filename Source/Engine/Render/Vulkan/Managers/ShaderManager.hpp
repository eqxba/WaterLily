#pragma once

#include "Engine/Render/Vulkan/Shaders/ShaderModule.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderCompiler.hpp"
#include "Engine/FileSystem/FileSystem.hpp"

class VulkanContext;

class ShaderManager
{
public:
    explicit ShaderManager(const VulkanContext& vulkanContext);

    ShaderModule CreateShaderModule(const FilePath& path, VkShaderStageFlagBits shaderStage,
        std::span<const ShaderDefine> shaderDefines, bool useCacheOnFailure = true) const;

private:
    ShaderModule CreateShaderModule(std::span<const uint32_t> spirvCode, VkShaderStageFlagBits shaderStage) const;
    
    const VulkanContext& vulkanContext;

    ShaderCompiler shaderCompiler;
};
