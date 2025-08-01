#pragma once

#include "Engine/Render/Vulkan/Shaders/ShaderModule.hpp"

namespace ShaderUtils
{
    void InsertDefines(std::string& glslCode, const std::vector<ShaderDefine>& shaderDefines);
    ShaderReflection GenerateReflection(const std::span<const uint32_t> spirvCode, VkShaderStageFlagBits shaderStage);
    std::vector<DescriptorSetReflection> MergeReflections(const std::vector<ShaderModule>& shaderModules);
}
