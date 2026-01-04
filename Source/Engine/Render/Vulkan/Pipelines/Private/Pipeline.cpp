#include "Engine/Render/Vulkan/Pipelines/Pipeline.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

Pipeline::Pipeline(const VkPipeline aPipeline, PipelineData pipelineData, const VulkanContext& aVulkanContext)
    : vulkanContext{ &aVulkanContext }
    , pipeline{ aPipeline }
    , layout{ pipelineData.layout }
    , type{ pipelineData.type }
    , setReflections{ std::move(pipelineData.setReflections) }
    , setLayouts{ std::move(pipelineData.setLayouts) }
    , pushConstants{ std::move(pipelineData.pushConstants) }
    , specializationConstants{ std::move(pipelineData.specializationConstants) }
{
    Assert(IsValid());
    Assert(setReflections.size() == setLayouts.size());
}

Pipeline::~Pipeline()
{
    // TODO: Deletion queue? Don't care now
    if (pipeline != VK_NULL_HANDLE)
    {
        Assert(layout != VK_NULL_HANDLE);

        const Device& device = vulkanContext->GetDevice();

        device.WaitIdle();

        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyPipelineLayout(device, layout, nullptr);
    }
}

Pipeline::Pipeline(Pipeline&& other) noexcept
    : vulkanContext{ other.vulkanContext }
    , pipeline{ other.pipeline }
    , layout{ other.layout }
    , type{ other.type }
    , setReflections{ std::move(other.setReflections) }
    , setLayouts{ std::move(other.setLayouts) }
    , pushConstants{ std::move(other.pushConstants) }
    , specializationConstants{ std::move(other.specializationConstants) }
{
    other.vulkanContext = nullptr;
    other.pipeline = VK_NULL_HANDLE;
    other.layout = VK_NULL_HANDLE;
}

Pipeline& Pipeline::operator=(Pipeline&& other) noexcept
{
    if (this != &other)
    {
        std::swap(vulkanContext, other.vulkanContext);
        std::swap(pipeline, other.pipeline);
        std::swap(layout, other.layout);
        std::swap(type, other.type);
        std::swap(setReflections, other.setReflections);
        std::swap(setLayouts, other.setLayouts);
        std::swap(pushConstants, other.pushConstants);
        std::swap(specializationConstants, other.specializationConstants);
    }

    return *this;
}

bool Pipeline::HasBinding(const std::string_view name) const
{
    Assert(IsValid());
    
    return std::ranges::any_of(setReflections, [&](const DescriptorSetReflection& set) {
        return std::ranges::any_of(set.bindings, [&](const BindingReflection& b) { return std::ranges::contains(b.names, name); });
    });
}
