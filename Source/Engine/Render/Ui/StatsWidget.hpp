#pragma once

#include "Engine/Render/Ui/Widget.hpp"

class VulkanContext;

class StatsWidget : public Widget
{
public:
    StatsWidget(const VulkanContext& vulkanContext);
    
    void Process(float deltaSeconds) override;
    void Build() override;
    
    bool IsAlwaysVisible() override
    {
        return true;
    }
    
private:
    const VulkanContext* vulkanContext = nullptr;
    
    std::array<float, 50> frameTimes = {};
};
