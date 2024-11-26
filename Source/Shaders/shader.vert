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

layout(push_constant) uniform PushConstants 
{
    mat4 model; 
} pushConstants;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUv;
layout (location = 2) out vec3 outColor;
layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;
layout (location = 5) out vec4 outTangent;

void main() 
{
    outUv = inUv;
	outColor = inColor;
	outTangent = inTangent;
    
    vec4 pos = ubo.projection * ubo.view * pushConstants.model * vec4(inPos, 1.0);

    outNormal = mat3(pushConstants.model) * inNormal;

	gl_Position = pos;
	
    vec3 lightPos = vec3(0.0, 1000.0, 0.0);

	outLightVec = lightPos - pos.xyz;
    outViewVec = ubo.viewPos.xyz - pos.xyz;
}