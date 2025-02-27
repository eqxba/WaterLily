#pragma once

#include <volk.h>

class VulkanContext;

enum class PipelineType
{
    eGraphics,
    eCompute,
};

class Pipeline
{
public:
    Pipeline() = default;
    Pipeline(VkPipeline pipeline, VkPipelineLayout layout, PipelineType type, const VulkanContext& vulkanContext);
    ~Pipeline();

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;

    Pipeline(Pipeline&& other) noexcept;
    Pipeline& operator=(Pipeline&& other) noexcept;
    
    VkPipeline Get() const
    {
        return pipeline;
    }

    VkPipelineLayout GetLayout() const
    {
        return layout;
    }
    
    PipelineType GetType() const
    {
        return type;
    }

    bool IsValid() const
    {
        return layout != VK_NULL_HANDLE && pipeline != VK_NULL_HANDLE;
    }

private:
    const VulkanContext* vulkanContext = nullptr;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    PipelineType type = PipelineType::eGraphics;
};
