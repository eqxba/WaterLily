#pragma once

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
    
    std::array<float, 50> frameTimes = {};
    uint64_t triangleCount = 0;
};
