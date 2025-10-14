#pragma once

#include <volk.h>

#include "Engine/Render/Vulkan/DescriptorSets/DescriptorSetLayout.hpp"
#include "Engine/Render/Vulkan/DescriptorSets/DescriptorSetReflection.hpp"

class VulkanContext;

enum class PipelineType
{
    eGraphics,
    eCompute,
};

struct SpecializationConstant
{
    std::string_view name;
    std::variant<VkBool32, int, float> value;
};

struct PipelineData
{
    VkPipelineLayout layout;
    PipelineType type;
    std::vector<DescriptorSetReflection> setReflections;
    std::vector<DescriptorSetLayout> setLayouts;
    std::unordered_map<std::string, VkPushConstantRange> pushConstants;
    std::vector<SpecializationConstant> specializationConstants;
};

class Pipeline
{
public:
    Pipeline() = default;
    Pipeline(VkPipeline pipeline, PipelineData pipelineData, const VulkanContext& vulkanContext);
    ~Pipeline();

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;

    Pipeline(Pipeline&& other) noexcept;
    Pipeline& operator=(Pipeline&& other) noexcept;
    
    bool HasBinding(std::string_view name) const;

    VkPipelineLayout GetLayout() const
    {
        return layout;
    }
    
    PipelineType GetType() const
    {
        return type;
    }
    
    const std::vector<DescriptorSetReflection>& GetSetReflections() const
    {
        return setReflections;
    }
    
    const std::vector<DescriptorSetLayout>& GetSetLayouts() const
    {
        return setLayouts;
    }
    
    const std::unordered_map<std::string, VkPushConstantRange>& GetPushConstants() const
    {
        return pushConstants;
    }
    
    const std::vector<SpecializationConstant>& GetSpecializationConstants() const
    {
        return specializationConstants;
    }

    bool IsValid() const
    {
        return layout != VK_NULL_HANDLE && pipeline != VK_NULL_HANDLE;
    }
    
    operator VkPipeline() const
    {
        return pipeline;
    }

private:
    const VulkanContext* vulkanContext = nullptr;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    PipelineType type = PipelineType::eGraphics;
    
    std::vector<DescriptorSetReflection> setReflections;
    std::vector<DescriptorSetLayout> setLayouts;
    std::unordered_map<std::string, VkPushConstantRange> pushConstants;
    std::vector<SpecializationConstant> specializationConstants;
};
