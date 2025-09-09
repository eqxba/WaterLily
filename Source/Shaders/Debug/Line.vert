#version 450

#extension GL_GOOGLE_include_directive : require

#include "Common.h"
#include "Math.glsl"

layout(location = 0) in vec3 inPos;

layout(push_constant) uniform Globals
{
    PushConstants globals;
};

void main()
{
    vec3 position = inPos;

    vec4 clip = globals.projection * globals.view * vec4(position, 1.0);

    gl_Position = clip;
}