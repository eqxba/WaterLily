#pragma once

#include "Shaders/Common.h"
#include "Utils/Constants.hpp"
#include "Engine/Render/Vulkan/Buffer/Buffer.hpp"
#include "Engine/Render/Vulkan/Image/RenderTarget.hpp"

struct RenderContext
{
    RenderTarget colorTarget;
    RenderTarget depthTarget;

    gpu::PushConstants globals = { .view = Matrix4::identity, .projection = Matrix4::identity };

    // Vertex pipeline
    Buffer vertexBuffer;
    Buffer indexBuffer;

    // Mesh pipeline
    Buffer meshletDataBuffer;
    Buffer meshletBuffer;

    Buffer primitiveBuffer;

    Buffer drawBuffer;

    Buffer commandCountBuffer;
    Buffer commandBuffer; // Either indirect commands or task commands, see PrimitiveCull.comp & PrimitiveCullStage
};
