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

    ComputePipelineBuilder& SetShaderModule(const ShaderModule* shaderModule);
    ComputePipelineBuilder& SetSpecializationConstants(std::vector<SpecializationConstant> specializationConstants);

private:
    const VulkanContext* vulkanContext = nullptr;

    const ShaderModule* shaderModule = nullptr;
    std::vector<SpecializationConstant> specializationConstants;
};
