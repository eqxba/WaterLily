#pragma once

#include "Shaders/Common.h"
#include "Utils/Constants.hpp"
#include "Engine/Render/RenderOptions.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/Buffer/Buffer.hpp"
#include "Engine/Render/Vulkan/Image/RenderTarget.hpp"
#include "Engine/Render/Vulkan/Managers/ShaderManager.hpp"

struct DebugData
{
    std::array<glm::vec3, 8> frustumCornersWorld;
};

struct RenderContext
{
    // For 2-pass occlusion culling
    RenderPass firstRenderPass;
    RenderPass secondRenderPass;
    
    RenderTarget colorTarget;
    RenderTarget depthTarget;
    RenderTarget depthResolveTarget;
    
    std::vector<VkFramebuffer> firstPassFramebuffers;
    std::vector<VkFramebuffer> secondPassFramebuffers;

    std::unordered_map<std::string_view, std::function<int()>> runtimeDefineGetters;
    gpu::PushConstants globals = { .view = Matrix4::identity, .projection = Matrix4::identity };
    
    // Vertex pipeline
    Buffer vertexBuffer;
    Buffer indexBuffer;

    // Mesh pipeline
    Buffer meshletDataBuffer;
    Buffer meshletBuffer;

    Buffer primitiveBuffer;

    Buffer drawBuffer;
    Buffer drawsVisibilityBuffer;
    Buffer drawsDebugDataBuffer;

    Buffer commandCountBuffer;
    Buffer commandBuffer; // Either indirect commands or task commands, see PrimitiveCull.comp & PrimitiveCullStage
    
    DebugData debugData;
};
