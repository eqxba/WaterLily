#pragma once

#include <volk.h>

#include "Engine/Render/Vulkan/Pipelines/Pipeline.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderModule.hpp"

class VulkanContext;

class ComputePipelineBuilder
{
public:
    // TODO: Add pipeline cache and manager
    ComputePipelineBuilder(const VulkanContext& vulkanContext);
    ~ComputePipelineBuilder() = default;

    ComputePipelineBuilder(const ComputePipelineBuilder&) = delete;
    ComputePipelineBuilder& operator=(const ComputePipelineBuilder&) = delete;

    ComputePipelineBuilder(ComputePipelineBuilder&& other) = delete;
    ComputePipelineBuilder& operator=(ComputePipelineBuilder&& other) = delete;

    Pipeline Build() const;

    // TODO: Get set layouts and push constants from reflection
    ComputePipelineBuilder& AddPushConstantRange(VkPushConstantRange pushConstantRange); // Temp!

    ComputePipelineBuilder& SetShaderModule(ShaderModule&& shaderModule);

private:
    const VulkanContext* vulkanContext = nullptr;

    // Temp! (parse from shaders)
    std::vector<VkPushConstantRange> pushConstantRanges;

    std::unique_ptr<ShaderModule> shaderModule;
};
