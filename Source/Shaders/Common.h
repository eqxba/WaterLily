#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

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