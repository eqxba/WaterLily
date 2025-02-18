#version 450

layout(set = 1, binding = 1) uniform texture2D texImage;
layout(set = 1, binding = 2) uniform sampler texSampler;

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec3 inViewVec;
layout(location = 4) in vec3 inLightVec;
layout(location = 5) in vec4 inTangent;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 color = texture(sampler2D(texImage, texSampler), inUv);

    vec3 N = normalize(inNormal);
    vec3 L = normalize(inLightVec);
    vec3 V = normalize(inViewVec);
    vec3 R = reflect(-L, N);

    const float ambient = 0.1;
    vec3 diffuse = max(dot(N, L), ambient).rrr;
    float specular = pow(max(dot(R, V), 0.0), 32.0);

    // outColor = vec4(diffuse * color.rgb + specular, color.a);
    outColor = vec4((diffuse * color.rgb + specular) * vec3(1.0, 0.0, 0.0), color.a);
}
