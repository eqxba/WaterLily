#version 450

#extension GL_GOOGLE_include_directive : require

#include "Common.h"
#include "Math.glsl"

layout(location = 0) in vec3 inPos;

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
    vec3 position = inPos;

    Draw draw = draws[gl_InstanceIndex];
    Primitive primitive = primitives[draw.primitiveIndex];

    vec3 primitiveCenterWorldPos = rotateQuat(primitive.center, draw.rotation) * draw.scale + draw.position;
    position = position * primitive.radius * draw.scale + primitiveCenterWorldPos;

    vec4 clip = globals.projection * globals.view * vec4(position, 1.0);

    gl_Position = clip;
}