#pragma once

#include "Engine/Render/Ui/Widget.hpp"

class VulkanContext;

class SettingsWidget : public Widget
{
public:
    SettingsWidget(const VulkanContext& vulkanContext);
    
    void Process(float deltaSeconds) override;
    void Build() override;
    
private:
DISABLE_WARNINGS_BEGIN
    const VulkanContext* vulkanContext = nullptr;
DISABLE_WARNINGS_END
};
