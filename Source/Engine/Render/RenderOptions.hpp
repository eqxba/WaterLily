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
    bool IsGraphicsPipelineTypeSupported(GraphicsPipelineType graphicsPipelineType) const;
    
    // Getters and setters
    RendererType GetRendererType() const;
    void SetRendererType(RendererType rendererType);

    GraphicsPipelineType GetGraphicsPipelineType() const;
    void SetGraphicsPipelineType(GraphicsPipelineType graphicsPipelineType);

    bool GetUseLods() const;
    void SetUseLods(bool useLods);
    
    bool GetVisualizeLods() const;
    void SetVisualizeLods(bool visualizeLods);

    bool GetFreezeCamera() const;
    void SetFreezeCamera(bool freezeCamera);
    
    uint32_t GetCurrentDrawCount() const;
    void SetCurrentDrawCount(uint32_t currentDrawCount);
    
    uint32_t GetMaxDrawCount() const;
    void SetMaxDrawCount(uint32_t maxDrawCount);
    
    template<typename Option, typename Event>
    void SetOption(Option& value, const Option newValue);
    
private:
    void OnKeyInput(const ES::KeyInput& event);
    
    const VulkanContext* vulkanContext = nullptr;
    
    EventSystem* eventSystem = nullptr;
    
    RendererType rendererType = RendererType::eScene;
    GraphicsPipelineType graphicsPipelineType = GraphicsPipelineType::eVertex;
    bool useLods = true;
    bool visualizeLods = false;
    bool freezeCamera = false;
    
    uint32_t currentDrawCount = gpu::primitiveCullMaxCommands;
    uint32_t maxDrawCount = gpu::primitiveCullMaxCommands;
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
