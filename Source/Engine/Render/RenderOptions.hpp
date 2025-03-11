#pragma once

enum class RenderPipeline
{
    eMesh = 0,
    eVertex,
};

namespace RenderOptions
{
    extern int renderer;
    extern RenderPipeline pipeline;
}
