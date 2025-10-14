#pragma once

enum class RendererType
{
    eForward = 0,
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
    inline constexpr std::array rendererTypes = { RendererType::eForward, RendererType::eCompute };
    inline constexpr std::array graphicsPipelineTypes = { GraphicsPipelineType::eMesh, GraphicsPipelineType::eVertex };
    inline constexpr std::array msaaSampleCounts = { VK_SAMPLE_COUNT_1_BIT, VK_SAMPLE_COUNT_2_BIT, VK_SAMPLE_COUNT_4_BIT,
        VK_SAMPLE_COUNT_8_BIT, VK_SAMPLE_COUNT_16_BIT, VK_SAMPLE_COUNT_32_BIT, VK_SAMPLE_COUNT_64_BIT };
}

// Helper macro for boilerplate getters, setters and change event structs
#define RENDER_OPTION(Name, Type, InitialValue, SupportFunction)                             \
public:                                                                                      \
    struct Name##Changed {};                                                                 \
                                                                                             \
    Type Get##Name() const                                                                   \
    {                                                                                        \
        return _##Name;                                                                      \
    }                                                                                        \
                                                                                             \
    void Set##Name(const Type value)                                                         \
    {                                                                                        \
        if (!SupportFunction(value))                                                         \
        {                                                                                    \
            return;                                                                          \
        }                                                                                    \
                                                                                             \
        SetOption<Type, Name##Changed>(_##Name, value);                                      \
    }                                                                                        \
                                                                                             \
private:                                                                                     \
    Type _##Name = InitialValue;
