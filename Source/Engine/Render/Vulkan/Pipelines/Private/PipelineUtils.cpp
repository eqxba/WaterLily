#include "Engine/Render/Vulkan/Pipelines/PipelineUtils.hpp"

void PipelineUtils::PushConstants(const VkCommandBuffer commandBuffer, const Pipeline& pipeline,
    const std::string& name, const std::span<const std::byte> data)
{
    const std::unordered_map<std::string, VkPushConstantRange>& pushConstants = pipeline.GetPushConstants();
    const auto it = pushConstants.find(name);
    
    Assert(it != pushConstants.end());
    
    const VkPushConstantRange& range = it->second;
    
    vkCmdPushConstants(commandBuffer, pipeline.GetLayout(), range.stageFlags, range.offset,
        static_cast<uint32_t>(data.size()), data.data());
}

uint32_t PipelineUtils::GroupCount(const uint32_t threadCount, const uint32_t groupSize)
{
    return static_cast<uint32_t>(std::ceil(static_cast<float>(threadCount) / static_cast<float>(groupSize)));
}
