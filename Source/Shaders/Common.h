#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
#pragma once

DISABLE_WARNINGS_BEGIN
#include <glm/glm.hpp>
DISABLE_WARNINGS_END

#include "Shaders/Config.h"

namespace gpu 
{
    using namespace glm;
    using uint = uint32_t;

#else
#extension GL_EXT_shader_8bit_storage: require
#extension GL_EXT_shader_16bit_storage: require
#extension GL_GOOGLE_include_directive: require

#include "Config.h"
#endif

struct CullData
{
    mat4 view;
    // Frustum planes' components in view space
    float frustumRightX;
    float frustumRightZ;
    float frustumTopY;
    float frustumTopZ;
    float near;
};

struct PushConstants 
{
    mat4 view;
    mat4 projection;
    uint drawCount;
    uint bUseLods;
    float lodTarget; // lod target error at z = 1
    uint padding;
    CullData cullData;
};

// TODO: Use positions only for shadows pass: measure impact and try to separate, do the packing, now 64 bytes / vertex
struct Vertex
{
    vec4 posAndU;
    vec4 normalAndV;
    vec4 tangent;
    vec4 color;
};

// Meshlet data buffer contains 2 important geometry elements for each meshlet stored one after another:
// 1. uint16_t / uint32_t offsets (check bShortVertexOffsets flag) to meshlet vertices in global vertex buffer.
// These offsets are calculated relative to meshlet firstVertexOffset, so meshlet vertices can be obtained as
// vertexBuffer[firstVertexOffset + vertexOffsets[0]]...vertexBuffer[firstVertexOffset + vertexOffsets[vertexCount - 1]]
// 2. Triangle indices which are used as gl_PrimitiveTriangleIndicesEXT output to produce triangles for meshlets.
// Triangle indices are stored as uint8_t elements.
struct Meshlet
{
    uint dataOffset;
    uint firstVertexOffset;
    uint8_t vertexCount;
    uint8_t triangleCount;
    uint8_t bShortVertexOffsets;
    uint8_t padding1;
    uint padding2;
};

struct Lod
{
    uint indexOffset;
    uint indexCount;
    uint meshletOffset;
    uint meshletCount;
    float error;
};

struct Primitive
{
    vec3 center;
    float radius;

    uint vertexOffset;
    uint vertexCount;

    uint lodCount;
    Lod lods[MAX_LOD_COUNT];
    uint padding;
};

struct Draw // Per individual thread in PrimitiveCull workgroup, the "highest level" draw
{
    vec3 position;
    float scale;
    vec4 rotation;

    uint primitiveIndex;
    // material index, etc.
    uint padding1;
    uint padding2;
    uint padding3; // TODO: Fix paddings
};

struct VkDrawIndexedIndirectCommand
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    uint vertexOffset;
    uint firstInstance;
};

struct TaskCommand
{
    uint drawIndex;
    uint meshletOffset;
    uint meshletCount;
    uint padding;
};

struct TaskPayload
{
    uint drawIndex;
    uint meshletOffset;
};

#ifdef __cplusplus
}
#endif

#endif
