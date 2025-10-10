#pragma once

#include "Shaders/Common.h"
#include "Engine/EventSystem.hpp"
#include "Engine/Render/RenderOptionsTypes.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

class RenderOptions
{
public:
    static void Initialize(const VulkanContext& vulkanContext, EventSystem& eventSystem);
    static void Deinitialize();
    
    static RenderOptions& Get();
    
    RenderOptions(const VulkanContext& vulkanContext, EventSystem& eventSystem);
    ~RenderOptions();

    RenderOptions(const RenderOptions&) = delete;
    RenderOptions& operator=(const RenderOptions&) = delete;

    RenderOptions(RenderOptions&&) = delete;
    RenderOptions& operator=(RenderOptions&&) = delete;

    // Support functions
    constexpr bool AlwaysSupported(std::any) { return true; }
    bool IsGraphicsPipelineTypeSupported(GraphicsPipelineType graphicsPipelineType) const;
    bool IsMsaaSampleCountSupported(VkSampleCountFlagBits sampleCount) const;
    
    // Getters and setters
    RENDER_OPTION(VSync, bool, true, AlwaysSupported)
    RENDER_OPTION(RendererType, RendererType, RendererType::eScene, AlwaysSupported)
    RENDER_OPTION(GraphicsPipelineType, GraphicsPipelineType, GraphicsPipelineType::eVertex, IsGraphicsPipelineTypeSupported)
    RENDER_OPTION(UseLods, bool, true, AlwaysSupported)
    RENDER_OPTION(VisualizeLods, bool, false, AlwaysSupported)
    RENDER_OPTION(FreezeCamera, bool, false, AlwaysSupported)
    RENDER_OPTION(CurrentDrawCount, uint32_t, gpu::primitiveCullMaxCommands, AlwaysSupported)
    RENDER_OPTION(MaxDrawCount, uint32_t, gpu::primitiveCullMaxCommands, AlwaysSupported)
    RENDER_OPTION(MsaaSampleCount, VkSampleCountFlagBits, VK_SAMPLE_COUNT_1_BIT, IsMsaaSampleCountSupported)
    RENDER_OPTION(VisualizeBoundingSpheres, bool, false, AlwaysSupported)
    RENDER_OPTION(VisualizeBoundingRectangles, bool, false, AlwaysSupported)
    RENDER_OPTION(ShowExtraGpuTimings, bool, false, AlwaysSupported)
    RENDER_OPTION(VisualizeDepth, bool, false, AlwaysSupported)
    RENDER_OPTION(DepthMipToVisualize, uint32_t, 0, AlwaysSupported)
    RENDER_OPTION(MaxDepthMipToVisualize, uint32_t, 0, AlwaysSupported) // TODO: For UI, we need better max limit solution
    
    // Just for random occasional tests
    RENDER_OPTION(TestBool, bool, false, AlwaysSupported)

private:
    template<typename Option, typename Event>
    void SetOption(Option& value, const Option newValue);
    
    void OnKeyInput(const ES::KeyInput& event);
    
    const VulkanContext* vulkanContext = nullptr;
    EventSystem* eventSystem = nullptr;
};

template<typename Option, typename Event>
void RenderOptions::SetOption(Option& value, const Option newValue)
{
    if (value != newValue)
    {
        value = newValue;
        eventSystem->Fire<Event>();
    }
}
