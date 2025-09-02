#pragma once

DISABLE_WARNINGS_BEGIN
#include <glm/glm.hpp>
DISABLE_WARNINGS_END

#include "Utils/DataStructures.hpp"

namespace Math
{
    glm::mat4 Perspective(float verticalFov, float aspectRatio, float near, float far, bool reverseDepth = true);
    glm::mat4 PerspectiveInfinite(float verticalFov, float aspectRatio, float near, bool reverseDepth = true);

    std::pair<std::vector<glm::vec3>, std::vector<uint32_t>> GenerateSphereMesh(uint32_t stacks, uint32_t sectors);

    Sphere Welzl(std::vector<glm::vec3> points);
}