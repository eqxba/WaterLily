#pragma once

#include "Engine/Render/Vulkan/Pipelines/Pipeline.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderModule.hpp"

namespace ShaderUtils
{
    void InsertDefines(std::string& glslCode, const std::vector<ShaderDefine>& shaderDefines);

    ShaderReflection GenerateReflection(const std::span<const uint32_t> spirvCode, VkShaderStageFlagBits shaderStage);
    std::vector<DescriptorSetReflection> MergeReflections(const std::vector<ShaderModule>& shaderModules);
    
    std::pair<std::vector<VkSpecializationMapEntry>, std::vector<std::byte>> CreateSpecializationData(const ShaderModule& shaderModule,
        const std::vector<SpecializationConstant>& specializationConstants);

    ShaderInstance CreateShaderInstance(const ShaderModule& shaderModule, const std::vector<SpecializationConstant>& specializationConstants);

    VkPipelineShaderStageCreateInfo GetShaderStageCreateInfo(const ShaderInstance& shaderInstance);
}
