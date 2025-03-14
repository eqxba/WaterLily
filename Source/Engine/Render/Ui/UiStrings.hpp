#pragma once

#include "Engine/Render/RenderOptions.hpp"

namespace UiStrings
{
    inline constexpr std::string_view placeholder = "Placeholder";

    template <typename T>
    constexpr std::string_view ToString(T) = delete;

    template <>
    constexpr std::string_view ToString<RendererType>(RendererType rendererType)
    {
        switch (rendererType)
        {
            case RendererType::eScene: return "Scene";
            case RendererType::eCompute: return "Compute";
        }
        
        return placeholder;
    }

    template <>
    constexpr std::string_view ToString<GraphicsPipelineType>(GraphicsPipelineType graphicsPipelineType)
    {
        switch (graphicsPipelineType)
        {
            case GraphicsPipelineType::eMesh: return "Mesh";
            case GraphicsPipelineType::eVertex: return "Vertex";
        }
        
        return placeholder;
    }
}
