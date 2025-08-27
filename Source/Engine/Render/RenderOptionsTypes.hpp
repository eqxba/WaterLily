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