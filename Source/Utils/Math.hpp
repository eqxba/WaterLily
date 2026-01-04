#pragma once

DISABLE_WARNINGS_BEGIN
#include <glm/glm.hpp>
DISABLE_WARNINGS_END

#include "Utils/DataStructures.hpp"

namespace Math
{
    glm::mat4 Perspective(float verticalFov, float aspectRatio, float near, float far, bool reverseDepth = true);
    glm::mat4 PerspectiveInfinite(float verticalFov, float aspectRatio, float near, bool reverseDepth = true);

    Sphere Welzl(std::vector<glm::vec3> points);
    Sphere AverageSphere(const std::vector<glm::vec3>& points);

    glm::vec4 NormalizePlane(const glm::vec4 plane);
}
