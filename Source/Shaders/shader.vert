#version 450

#include "Common.h"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUv;
layout(location = 3) in vec3 inColor;
layout(location = 4) in vec4 inTangent;

layout(binding = 0) uniform Ubo 
{
    UniformBufferObject ubo; 
};

layout(std430, binding = 2) readonly buffer TransformsBuffer 
{
    mat4 transforms[];
};

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUv;
layout (location = 2) out vec3 outColor;
layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;
layout (location = 5) out vec4 outTangent;

vec3 hashColor(uint seed) 
{
    uint m = 1664525u;
    uint a = 1013904223u;
    seed = seed * m + a;
    return vec3(float((seed >> 16) & 255) / 255.0, float((seed >> 8) & 255) / 255.0, float(seed & 255) / 255.0);
}

void main() 
{
    outUv = inUv;
    outColor = hashColor(gl_InstanceIndex);
    outTangent = inTangent;

    mat4 transform = transforms[gl_InstanceIndex];
    
    vec4 pos = ubo.projection * ubo.view * transform * vec4(inPos, 1.0);

    outNormal = mat3(transform) * inNormal;

    gl_Position = pos;
    
    vec3 lightPos = vec3(0.0, 1000.0, 0.0);

    outLightVec = lightPos - pos.xyz;
    outViewVec = ubo.viewPos.xyz - pos.xyz;
}