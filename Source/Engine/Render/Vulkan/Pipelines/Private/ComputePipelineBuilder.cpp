#include "Engine/Render/Vulkan/Pipelines/ComputePipelineBuilder.hpp"

#include "Engine/Render/Vulkan/VulkanUtils.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderUtils.hpp"

ComputePipelineBuilder::ComputePipelineBuilder(const VulkanContext& aVulkanContext)
    : vulkanContext{&aVulkanContext}
{}

Pipeline ComputePipelineBuilder::Build() const
{
    using namespace VulkanUtils;
    using namespace ShaderUtils;

    Assert(shaderModule);
    
    if (!shaderModule->IsValid())
    {
        return {};
    }
    
    const VkDevice device = vulkanContext->GetDevice();
    const ShaderReflection& reflection = shaderModule->GetReflection();
    
    std::vector<DescriptorSetLayout> setLayouts = CreateDescriptorSetLayouts(reflection.descriptorSets, *vulkanContext);
    std::vector<VkPushConstantRange> pushConstantRanges = GetPushConstantRanges(reflection.pushConstants);
    
    const VkPipelineLayout pipelineLayout = CreatePipelineLayout(setLayouts, pushConstantRanges, device);
    
    ShaderInstance shaderInstance = CreateShaderInstance(*shaderModule, specializationConstants);
    
    VkComputePipelineCreateInfo pipelineInfo = { .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
    pipelineInfo.stage = GetShaderStageCreateInfo(shaderInstance);
    pipelineInfo.layout = pipelineLayout;
    
    // Can be used to create pipeline from similar one (which is faster than entirely new one)
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VkPipeline pipeline;
    const VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
    Assert(result == VK_SUCCESS);
    
    PipelineData pipelineData = {
        .layout = pipelineLayout,
        .type = PipelineType::eCompute,
        .setReflections = reflection.descriptorSets,
        .setLayouts = std::move(setLayouts),
        .pushConstants = reflection.pushConstants,
        .specializationConstants = std::move(specializationConstants) };

    return { pipeline, std::move(pipelineData), *vulkanContext };
}

ComputePipelineBuilder& ComputePipelineBuilder::SetShaderModule(const ShaderModule& aShaderModule)
{
    shaderModule = &aShaderModule;

    return *this;
}

ComputePipelineBuilder& ComputePipelineBuilder::SetSpecializationConstants(
    std::vector<SpecializationConstant> aSpecializationConstants)
{
    specializationConstants = std::move(aSpecializationConstants);

    return *this;
}
