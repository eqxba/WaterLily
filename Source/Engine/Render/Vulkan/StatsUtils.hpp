#pragma once

#include <volk.h>

class Device;
class VulkanContext;

enum class GpuTimestamp : uint32_t
{
    eFrameBegin,
    eFrameEnd,
    eFirstPassBegin,
    eFirstPassEnd,
    eDepthPyramidEnd,
    eSecondPassEnd,
    eCount
};

constexpr auto GpuTimestampsCount = static_cast<uint32_t>(GpuTimestamp::eCount);

struct FrameRenderStats
{
    uint64_t triangleCount = 0; // VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT
    uint64_t rawTimestamps[GpuTimestampsCount];
};

struct FrameQueryPools
{
    VkQueryPool pipelineStats = VK_NULL_HANDLE;
    VkQueryPool timestamps = VK_NULL_HANDLE;
};

enum class GpuTiming
{
    eFrame,
    eFirstPass,
    eDepthPyramid,
    eSecondPass,
};

namespace StatsUtils
{
    VkQueryPool CreateQueryPool(const VulkanContext& vulkanContext, VkQueryType queryType, uint32_t queryCount,
        VkQueryPipelineStatisticFlags pipelineStatisticFlags = 0);

    FrameQueryPools CreateFrameQueryPools(const VulkanContext& vulkanContext);
    void DestroyFrameQueryPools(FrameQueryPools& frameQueryPools, const VulkanContext& vulkanContext);
    
    void WriteTimestamp(VkCommandBuffer commandBuffer, VkQueryPool queryPool, GpuTimestamp timestamp);

    void ResetFrameQueryPools(VkCommandBuffer commandBuffer, const FrameQueryPools& frameQueryPools);

    void GatherFrameStats(VkDevice device, const FrameQueryPools& frameQueryPools, FrameRenderStats& renderStats);

    float GetTimingMs(const Device& device, const FrameRenderStats& renderStats, GpuTiming timingType);
}
