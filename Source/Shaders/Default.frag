#version 450

#extension GL_GOOGLE_include_directive: require

#include "Config.h"

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec4 inTangent;
layout(location = 2) in vec2 inUv;
layout(location = 3) in vec4 inColor;

// layout(set = 0, binding = 6) uniform texture2D texImage;
// layout(set = 0, binding = 7) uniform sampler texSampler;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 normal = normalize(inNormal);
    vec3 lightDir = normalize(vec3(0.5, 0.5, 1.0));

    float intensity = max(dot(normal, lightDir), 0.0);

    vec4 baseColor = vec4(normal, 1.0);
    vec3 diffuse = baseColor.rgb * intensity;
    vec3 ambient = baseColor.rgb * 0.2;

    #if DEBUG_VERTEX_COLOR
        outColor = inColor;
    #else
        outColor = vec4(diffuse + ambient, baseColor.a);
    #endif
}