#ifndef CONFIG_H
#define CONFIG_H

#define PRIMITIVE_CULL_WG_SIZE 64
#define PRIMITIVE_CULL_MAX_COMMANDS 4194304 // Based on maxTaskWorkGroupTotalCount for my 3060

#define TASK_WG_SIZE 64
#define MESH_WG_SIZE 64

#define MAX_LOD_COUNT 8

#define MAX_MESHLET_VERTICES 64
#define MAX_MESHLET_TRIANGLES 96

#ifdef __cplusplus
#pragma once

namespace gpu 
{   
    constexpr uint32_t primitiveCullWgSize = PRIMITIVE_CULL_WG_SIZE;
    constexpr uint32_t primitiveCullMaxCommands = PRIMITIVE_CULL_MAX_COMMANDS;

    constexpr uint32_t taskWgSize = TASK_WG_SIZE;
    constexpr uint32_t meshWgSize = MESH_WG_SIZE;

    constexpr uint32_t maxLodCount = MAX_LOD_COUNT;

    constexpr uint32_t maxMeshletVertices = MAX_MESHLET_VERTICES;
    constexpr uint32_t maxMeshletTriangles = MAX_MESHLET_TRIANGLES;
}
#endif

#endif