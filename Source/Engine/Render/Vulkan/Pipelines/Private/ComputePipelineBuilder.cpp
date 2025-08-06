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

    Assert(shaderModule != nullptr);
    
    const VkDevice device = vulkanContext->GetDevice();
    const ShaderReflection& reflection = shaderModule->GetReflection();
    
    std::vector<DescriptorSetLayout> setLayouts = CreateDescriptorSetLayouts(reflection.descriptorSets, *vulkanContext);
    
    const VkPipelineLayout pipelineLayout = CreatePipelineLayout(setLayouts, pushConstantRanges, device);
    
    VkComputePipelineCreateInfo pipelineInfo = { .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
    pipelineInfo.stage = shaderModule->GetVkPipelineShaderStageCreateInfo();
    pipelineInfo.layout = pipelineLayout;
    
    std::pair<std::vector<VkSpecializationMapEntry>, std::vector<std::byte>> specializationData = CreateSpecializationData(*shaderModule, specializationConstants);
    VkSpecializationInfo specializationInfo = {};
    
    if (const auto& [entries, data] = specializationData; !entries.empty())
    {
        specializationInfo = { static_cast<uint32_t>(entries.size()), entries.data(), data.size(), data.data() };
        pipelineInfo.stage.pSpecializationInfo = &specializationInfo;
    }
    
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
        .specializationConstants = std::move(specializationConstants) };

    return { pipeline, std::move(pipelineData), *vulkanContext };
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

ComputePipelineBuilder& ComputePipelineBuilder::SetSpecializationConstants(
    std::vector<SpecializationConstant> aSpecializationConstants)
{
    specializationConstants = std::move(aSpecializationConstants);

    return *this;
}
