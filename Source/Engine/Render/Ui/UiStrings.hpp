#pragma once

#include "Engine/Render/RenderOptions.hpp"

namespace UiStrings
{
    inline constexpr std::string_view placeholder = "Placeholder";

    template <typename T>
    constexpr std::string_view ToString(T) = delete;

    template <>
    constexpr std::string_view ToString<RendererType>(const RendererType rendererType)
    {
        switch (rendererType)
        {
            case RendererType::eScene: return "Scene";
            case RendererType::eCompute: return "Compute";
        }
        
        return placeholder;
    }

    template <>
    constexpr std::string_view ToString<GraphicsPipelineType>(const GraphicsPipelineType graphicsPipelineType)
    {
        switch (graphicsPipelineType)
        {
            case GraphicsPipelineType::eMesh: return "Mesh";
            case GraphicsPipelineType::eVertex: return "Vertex";
        }
        
        return placeholder;
    }

    template <>
    constexpr std::string_view ToString<VkSampleCountFlagBits>(const VkSampleCountFlagBits sampleCount)
    {
        switch (sampleCount)
        {
            case VK_SAMPLE_COUNT_1_BIT: return "1x";
            case VK_SAMPLE_COUNT_2_BIT: return "2x";
            case VK_SAMPLE_COUNT_4_BIT: return "4x";
            case VK_SAMPLE_COUNT_8_BIT: return "8x";
            case VK_SAMPLE_COUNT_16_BIT: return "16x";
            case VK_SAMPLE_COUNT_32_BIT: return "32x";
            case VK_SAMPLE_COUNT_64_BIT: return "64x";
            case VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM: return placeholder;
        }
    }
}
