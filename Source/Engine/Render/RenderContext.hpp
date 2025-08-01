#pragma once

#include "Shaders/Common.h"
#include "Utils/Constants.hpp"
#include "Engine/Render/RenderOptions.hpp"
#include "Engine/Render/Vulkan/Buffer/Buffer.hpp"
#include "Engine/Render/Vulkan/Image/RenderTarget.hpp"
#include "Engine/Render/Vulkan/Managers/ShaderManager.hpp"

struct RenderContext
{
    RenderTarget colorTarget;
    RenderTarget depthTarget;

    std::vector<ShaderDefine> globalDefines;
    gpu::PushConstants globals = { .view = Matrix4::identity, .projection = Matrix4::identity };

    // Vertex pipeline
    Buffer vertexBuffer;
    Buffer indexBuffer;

    // Mesh pipeline
    Buffer meshletDataBuffer;
    Buffer meshletBuffer;

    Buffer primitiveBuffer;

    Buffer drawBuffer;
    Buffer drawDebugDataBuffer;

    Buffer commandCountBuffer;
    Buffer commandBuffer; // Either indirect commands or task commands, see PrimitiveCull.comp & PrimitiveCullStage
    
    // Store this in a better way and use a callback on render option change
    bool visualizeLods = false;
    GraphicsPipelineType graphicsPipelineType = GraphicsPipelineType::eVertex;
};
