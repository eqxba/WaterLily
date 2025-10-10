#pragma once

#include "Engine/Render/Vulkan/Pipelines/Pipeline.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderModule.hpp"

namespace ShaderUtils
{
    void InsertDefines(std::string& glslCode, std::span<const ShaderDefine> shaderDefines);

    ShaderReflection GenerateReflection(const std::span<const uint32_t> spirvCode, VkShaderStageFlagBits shaderStage);

    std::vector<DescriptorSetReflection> MergeDescriptorSetReflections(std::span<const ShaderModule> shaderModules);
    std::unordered_map<std::string, VkPushConstantRange> MergePushConstantReflections(std::span<const ShaderModule> shaderModules);

    ShaderInstance CreateShaderInstance(const ShaderModule& shaderModule, const std::vector<SpecializationConstant>& specializationConstants);

    VkPipelineShaderStageCreateInfo GetShaderStageCreateInfo(const ShaderInstance& shaderInstance);
    // Makes new VkPushConstantRange from reflection ranges, new ranges are valid for pipeline layout creation
    std::vector<VkPushConstantRange> GetPushConstantRanges(const std::unordered_map<std::string, VkPushConstantRange>& reflectionRanges);
}
