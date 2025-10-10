#include "Engine/Render/Vulkan/StatsUtils.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace StatsUtilsDetails
{
    static void ResetQueryPool(const VulkanContext& vulkanContext, const VkQueryPool queryPool, const uint32_t queryCount,
        const bool dummyBeginEnd)
    {
        vulkanContext.GetDevice().ExecuteOneTimeCommandBuffer([&](const VkCommandBuffer cmd) {
            vkCmdResetQueryPool(cmd, queryPool, 0, queryCount);

            if (dummyBeginEnd) // Init with zero values
            {
                for (uint32_t i = 0; i < queryCount; ++i)
                {
                    // Initialize to have zero values
                    vkCmdBeginQuery(cmd, queryPool, i, 0);
                    vkCmdEndQuery(cmd, queryPool, i);
                }
            }
        });
    }
}

VkQueryPool StatsUtils::CreateQueryPool(const VulkanContext& vulkanContext, const VkQueryType queryType, const uint32_t queryCount,
    const VkQueryPipelineStatisticFlags pipelineStatisticFlags /* = 0 */)
{
    VkQueryPoolCreateInfo queryPoolInfo = { .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
    queryPoolInfo.queryType = queryType;
    queryPoolInfo.pipelineStatistics = pipelineStatisticFlags;
    queryPoolInfo.queryCount = queryCount;

    VkQueryPool queryPool;
    const VkResult result = vkCreateQueryPool(vulkanContext.GetDevice(), &queryPoolInfo, nullptr, &queryPool);
    Assert(result == VK_SUCCESS);
    
    StatsUtilsDetails::ResetQueryPool(vulkanContext, queryPool, queryCount, queryType != VK_QUERY_TYPE_TIMESTAMP);
    
    return queryPool;
}

FrameQueryPools StatsUtils::CreateFrameQueryPools(const VulkanContext& vulkanContext)
{
    FrameQueryPools frameQueryPools;
    
    if (vulkanContext.GetDevice().GetProperties().pipelineStatisticsQuerySupported)
    {
        frameQueryPools.pipelineStats = CreateQueryPool(vulkanContext, VK_QUERY_TYPE_PIPELINE_STATISTICS, 1, VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT);
    }
    
    frameQueryPools.timestamps = CreateQueryPool(vulkanContext, VK_QUERY_TYPE_TIMESTAMP, static_cast<uint32_t>(GpuTimestamp::eCount));
    
    return frameQueryPools;
}

void StatsUtils::DestroyFrameQueryPools(FrameQueryPools& frameQueryPools, const VulkanContext& vulkanContext)
{
    if (frameQueryPools.pipelineStats != VK_NULL_HANDLE)
    {
        vkDestroyQueryPool(vulkanContext.GetDevice(), frameQueryPools.pipelineStats, nullptr);
    }
    
    vkDestroyQueryPool(vulkanContext.GetDevice(), frameQueryPools.timestamps, nullptr);
}

void StatsUtils::WriteTimestamp(const VkCommandBuffer commandBuffer, const VkQueryPool queryPool, const GpuTimestamp timestamp)
{
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, queryPool, static_cast<uint32_t>(timestamp));
}

void StatsUtils::ResetFrameQueryPools(const VkCommandBuffer commandBuffer, const FrameQueryPools& frameQueryPools)
{
    if (frameQueryPools.pipelineStats != VK_NULL_HANDLE)
    {
        vkCmdResetQueryPool(commandBuffer, frameQueryPools.pipelineStats, 0, 1);
    }
    
    vkCmdResetQueryPool(commandBuffer, frameQueryPools.timestamps, 0, GpuTimestampsCount);
}

void StatsUtils::GatherFrameStats(const VkDevice device, const FrameQueryPools& frameQueryPools, FrameRenderStats& renderStats)
{
    if (frameQueryPools.pipelineStats != VK_NULL_HANDLE)
    {
        vkGetQueryPoolResults(device, frameQueryPools.pipelineStats, 0, 1, sizeof(renderStats.triangleCount),
            &renderStats.triangleCount, sizeof(renderStats.triangleCount), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    }
    
    vkGetQueryPoolResults(device, frameQueryPools.timestamps, 0, GpuTimestampsCount, sizeof(renderStats.rawTimestamps),
        &renderStats.rawTimestamps, sizeof(renderStats.rawTimestamps[0]), VK_QUERY_RESULT_64_BIT);
}

float StatsUtils::GetTimingMs(const Device& device, const FrameRenderStats& renderStats, const GpuTiming timingType)
{
    uint64_t rawBegin = 0;
    uint64_t rawEnd = 0;
    
    switch (timingType)
    {
    case GpuTiming::eFrame:
        rawBegin = renderStats.rawTimestamps[static_cast<uint32_t>(GpuTimestamp::eFrameBegin)];
        rawEnd = renderStats.rawTimestamps[static_cast<uint32_t>(GpuTimestamp::eFrameEnd)];
        break;
    case GpuTiming::eFirstPass:
        rawBegin = renderStats.rawTimestamps[static_cast<uint32_t>(GpuTimestamp::eFirstPassBegin)];
        rawEnd = renderStats.rawTimestamps[static_cast<uint32_t>(GpuTimestamp::eFirstPassEnd)];
        break;
    case GpuTiming::eDepthPyramid:
        rawBegin = renderStats.rawTimestamps[static_cast<uint32_t>(GpuTimestamp::eFirstPassEnd)];
        rawEnd = renderStats.rawTimestamps[static_cast<uint32_t>(GpuTimestamp::eDepthPyramidEnd)];
        break;
    case GpuTiming::eSecondPass:
        rawBegin = renderStats.rawTimestamps[static_cast<uint32_t>(GpuTimestamp::eDepthPyramidEnd)];
        rawEnd = renderStats.rawTimestamps[static_cast<uint32_t>(GpuTimestamp::eSecondPassEnd)];
        break;
    }
    
    const float devicePeriodNs = device.GetProperties().physicalProperties.limits.timestampPeriod;
    
    const auto beginMs = static_cast<double>(rawBegin) * devicePeriodNs * 1e-6;
    const auto endMs = static_cast<double>(rawEnd) * devicePeriodNs * 1e-6;
    
    return static_cast<float>(endMs - beginMs);
}
