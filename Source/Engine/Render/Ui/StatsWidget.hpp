#pragma once

#include "Utils/RingAccumulator.hpp"
#include "Engine/Render/Ui/Widget.hpp"

class VulkanContext;

class StatsWidget : public Widget
{
public:
    StatsWidget(const VulkanContext& vulkanContext);
    
    void Process(const Frame& frame, float deltaSeconds) override;
    void Build() override;
    
    bool IsAlwaysVisible() override
    {
        return true;
    }
    
private:
    const VulkanContext* vulkanContext = nullptr;
    
    RingAccumulator<float> cpuFrameTimesSeconds;
    RingAccumulator<float> gpuFrameTimesMs;
    
    // Extra GPU timings
    RingAccumulator<float> firstPassTimesMs;
    RingAccumulator<float> secondPassTimesMs;
    RingAccumulator<float> depthPyramidTimesMs;
    
    uint64_t triangleCount = 0;
};
