#include "Engine/Render/Vulkan/Pipelines/ComputePipelineBuilder.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanUtils.hpp"

ComputePipelineBuilder::ComputePipelineBuilder(const VulkanContext& aVulkanContext)
    : vulkanContext{&aVulkanContext}
{}

Pipeline ComputePipelineBuilder::Build() const
{
    using namespace VulkanUtils;

    Assert(shaderModule);
    
    const VkDevice device = vulkanContext->GetDevice();

    const VkPipelineLayout pipelineLayout = CreatePipelineLayout(descriptorSetLayouts, pushConstantRanges, device);
    
    VkComputePipelineCreateInfo pipelineInfo = { .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
    pipelineInfo.stage = shaderModule->GetVkPipelineShaderStageCreateInfo();
    pipelineInfo.layout = pipelineLayout;
    
    // Can be used to create pipeline from similar one (which is faster than entirely new one)
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VkPipeline pipeline;
    const VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
    Assert(result == VK_SUCCESS);

    return { pipeline, pipelineLayout, PipelineType::eCompute, *vulkanContext };
}

ComputePipelineBuilder& ComputePipelineBuilder::SetDescriptorSetLayouts(
    std::vector<VkDescriptorSetLayout> aDescriptorSetLayouts)
{
    descriptorSetLayouts = std::move(aDescriptorSetLayouts);

    return *this;
}

ComputePipelineBuilder& ComputePipelineBuilder::AddPushConstantRange(const VkPushConstantRange pushConstantRange)
{
    pushConstantRanges.push_back(pushConstantRange);

    return *this;
}

ComputePipelineBuilder& ComputePipelineBuilder::SetShaderModule(ShaderModule&& aShaderModule)
{
    shaderModule = std::make_unique<ShaderModule>(std::move(aShaderModule));

    return *this;
}
