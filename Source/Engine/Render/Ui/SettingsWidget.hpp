#pragma once

#include "Engine/Render/Ui/Widget.hpp"
#include "Engine/Render/RenderOptions.hpp"

class EventSystem;
class VulkanContext;

class SettingsWidget : public Widget
{
public:
    SettingsWidget(EventSystem& eventSystem, const VulkanContext& vulkanContext);
    virtual ~SettingsWidget() override;
    
    void Process(const Frame& frame, float deltaSeconds) override;
    void Build() override;
    
private:
    void OnVSyncChanged();
    void OnOcclusionCullingChanged();
    
DISABLE_WARNINGS_BEGIN
    const VulkanContext* vulkanContext = nullptr;
    EventSystem* eventSystem = nullptr;
DISABLE_WARNINGS_END
    
    RenderOptions* renderOptions = nullptr;
    std::vector<GraphicsPipelineType> supportedGraphicsPipelineTypes;
    std::vector<VkSampleCountFlagBits> supportedMsaaSampleCounts;
};
