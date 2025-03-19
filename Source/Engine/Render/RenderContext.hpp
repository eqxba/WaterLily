#pragma once

#include "Shaders/Common.h"
#include "Utils/Constants.hpp"
#include "Engine/Render/Vulkan/Buffer/Buffer.hpp"
#include "Engine/Render/Vulkan/Image/RenderTarget.hpp"
#include "Engine/Render/Vulkan/DescriptorSets/DescriptorSetLayout.hpp"

struct RenderContext
{
    RenderTarget colorTarget;
    RenderTarget depthTarget;

    gpu::PushConstants globals = { .view = Matrix4::identity, .projection = Matrix4::identity, .viewPos = Vector3::zero };

    // Regular pipeline
    Buffer vertexBuffer;
    Buffer indexBuffer;

    Buffer primitiveBuffer;

    Buffer drawBuffer;

    Buffer commandCountBuffer;
    Buffer indirectBuffer;

    // Meshlet pipeline
    // Buffer meshletIndirectBuffer (?)
    
    std::vector<VkDescriptorSet> globalDescriptors;
    DescriptorSetLayout globalDescriptorSetLayout;
};
