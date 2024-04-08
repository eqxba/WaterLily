#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace gpu 
{ 
    using namespace glm;
#endif

struct UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
};

#ifdef __cplusplus
}
#endif

#endif