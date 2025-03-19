#pragma once

#include "Shaders/Common.h"
#include "Utils/Constants.hpp"

#include <volk.h>

// TODO: Store in scene components separately: MeshComponent and TransformComponent
struct Mesh
{
    uint32_t firstPrimitiveIndex = 0;
    uint32_t primitiveCount = 0;

    glm::mat4 transform;
};

struct RawScene
{
    // GPU data
    std::vector<gpu::Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<uint32_t> meshletData;
    std::vector<gpu::Meshlet> meshlets;
    std::vector<gpu::Primitive> primitives;

    // CPU data
    std::vector<Mesh> meshes;
};