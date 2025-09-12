#version 450

#extension GL_GOOGLE_include_directive : require

#include "Common.h"
#include "Math.glsl"

layout(push_constant) uniform Globals
{
    PushConstants globals;
};

layout(set = 0, binding = 0) readonly buffer Draws 
{
    Draw draws[]; 
};

layout(set = 0, binding = 1) readonly buffer Primitives 
{
    Primitive primitives[]; 
};

void main()
{
    Draw draw = draws[gl_InstanceIndex];    
    Primitive primitive = primitives[draw.primitiveIndex];

    vec3 center = rotateQuat(primitive.center, draw.rotation) * draw.scale + draw.position;
    center = (globals.cullData.view * vec4(center, 1.0)).xyz;

    float radius = primitive.radius * draw.scale;

    vec4 lbrt;
    bool validExtents = sphereNdcExtents(center, radius, globals.projection[0][0], globals.projection[1][1], globals.cullData.near, lbrt);

    // (0)------(1)
    //  |      / |
    //  |    /   |
    //  |  /     |
    // (2)------(3)

    vec2 position = validExtents 
        ? vec2(lbrt[(gl_VertexIndex & 1) << 1], -lbrt[3 - (gl_VertexIndex & 2)]) // Flip Y, Vulkan NDC
        : vec2(-100.0); // Let it be clipped as it's invalid

    vec4 clip = vec4(position, 1.0, 1.0); // Reverse depth, draw on the near plane

    gl_Position = clip;
}