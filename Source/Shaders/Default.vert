#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_ARB_shader_draw_parameters: require

#include "Common.h"
#include "Math.glsl"

layout(location = 0) in vec4 inPosAndU;
layout(location = 1) in vec4 inNormalAndV;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec4 inColor;

layout(push_constant) uniform Globals
{
    PushConstants globals;
};

layout(set = 0, binding = 0) readonly buffer Draws 
{
    Draw draws[]; 
};

layout(set = 0, binding = 1) readonly buffer IndirectCommands
{
    IndirectCommand indirectCommands[];
};

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec4 outTangent;
layout(location = 2) out vec2 outUv;
layout(location = 3) out vec4 outColor;

void main() 
{
    vec3 position = inPosAndU.xyz;
    vec3 normal = inNormalAndV.xyz;
    vec4 tangent = inTangent;
    vec2 uv = vec2(inPosAndU.w, inNormalAndV.w);

    #if VISUALIZE_LODS
        vec4 color = hashToColor(hash(gl_InstanceIndex));
    #else
        vec4 color = inColor;
    #endif

    Draw draw = draws[indirectCommands[gl_DrawIDARB].drawIndex];

    position = rotateQuat(position, draw.rotation) * draw.scale + draw.position;
    normal = rotateQuat(normal, draw.rotation);    

    vec4 clip = globals.projection * globals.view * vec4(position, 1.0);

    gl_Position = clip;
    outNormal = normal;
    outTangent = tangent;
    outUv = uv;
    outColor = color;
}