#pragma once

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

namespace ES // Event system
{
    namespace RO // RenderOptions, helper types for render options change callbacks
    {
        struct RendererType {};
        struct GraphicsPipelineType {};
        struct VisualizeLods {};
    }
}
