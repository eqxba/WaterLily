#pragma once

DISABLE_WARNINGS_BEGIN
#include <glm/glm.hpp>
DISABLE_WARNINGS_END

#include "Utils/DataStructures.hpp"

namespace MeshUtils
{
    // By default here everything's unit radius
    std::pair<std::vector<glm::vec3>, std::vector<uint32_t>> GenerateSphereMesh(uint32_t stacks, uint32_t sectors);
    std::vector<glm::vec3> GenerateCircleLineStrip(uint32_t segments);
    std::vector<glm::vec3> GenerateCircleLineList(uint32_t segments);
}
