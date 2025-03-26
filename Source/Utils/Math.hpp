#pragma once

DISABLE_WARNINGS_BEGIN
#include <glm/glm.hpp>
DISABLE_WARNINGS_END

namespace Math
{
    glm::mat4 Perspective(float verticalFov, float aspectRatio, float near, float far, bool reverseDepth = true, 
        bool flipY = true);
}