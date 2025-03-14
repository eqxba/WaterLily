#pragma once

#include "Engine/Render/Vulkan/VulkanContext.hpp"

class EventSystem;

namespace ES
{
    struct KeyInput;
}

enum class RendererType
{
    eScene = 0,
    eCompute,
};

enum class GraphicsPipelineType
{
    eMesh = 0,
    eVertex,
};

namespace OptionValues
{
    // Other code might need these to iterate through
    inline constexpr std::array rendererTypes = { RendererType::eScene, RendererType::eCompute };
    inline constexpr std::array graphicsPipelineTypes = { GraphicsPipelineType::eMesh, GraphicsPipelineType::eVertex };
}

class RenderOptions
{
public:
    static void Initialize(const VulkanContext& vulkanContext, EventSystem& eventSystem);
    static RenderOptions& Get();
    
    RenderOptions(const VulkanContext& vulkanContext, EventSystem& eventSystem);
    ~RenderOptions();

    RenderOptions(const RenderOptions&) = delete;
    RenderOptions& operator=(const RenderOptions&) = delete;

    RenderOptions(RenderOptions&&) = delete;
    RenderOptions& operator=(RenderOptions&&) = delete;

    // Support functions
    bool IsGraphicsPipelineTypeSupported(GraphicsPipelineType graphicsPipelineType);
    
    // Getters and setters
    RendererType GetRendererType();
    void SetRendererType(RendererType rendererType);

    GraphicsPipelineType GetGraphicsPipelineType();
    void SetGraphicsPipelineType(GraphicsPipelineType graphicsPipelineType);
    
private:
    void OnKeyInput(const ES::KeyInput& event);
    
    const VulkanContext* vulkanContext = nullptr;
    
    EventSystem* eventSystem = nullptr;
    
    RendererType rendererType = RendererType::eScene;
    GraphicsPipelineType graphicsPipelineType = GraphicsPipelineType::eVertex;
};
