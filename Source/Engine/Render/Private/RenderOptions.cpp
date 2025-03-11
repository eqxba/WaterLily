#include "Engine/Render/RenderOptions.hpp"

namespace RenderOptions
{
    // TODO: CVar
    int renderer = 0;
    // 0 - scene renderer
    // 1 - compute renderer

    RenderPipeline pipeline = RenderPipeline::eMesh;
}
