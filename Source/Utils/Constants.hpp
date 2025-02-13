#pragma once

#include "Engine/FileSystem/FileSystem.hpp"

#include <glm/glm.hpp>

namespace Vector2
{
    constexpr auto zero = glm::vec2(0.0f, 0.0f);
    constexpr auto unitX = glm::vec2(1.0f, 0.0f);
    constexpr auto unitY = glm::vec2(0.0f, 1.0f);
    constexpr auto allOnes = glm::vec2(1.0f, 1.0f);
}

namespace Vector3
{
    constexpr auto zero = glm::vec3(0.0f, 0.0f, 0.0f);
    constexpr auto unitX = glm::vec3(1.0f, 0.0f, 0.0f);
    constexpr auto unitY = glm::vec3(0.0f, 1.0f, 0.0f);
    constexpr auto unitZ = glm::vec3(0.0f, 0.0f, 1.0f);
    constexpr auto allOnes = glm::vec3(1.0f, 1.0f, 1.0f);
}

namespace Vector4
{
    constexpr auto zero = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    constexpr auto unitX = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    constexpr auto unitY = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
    constexpr auto unitZ = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
    constexpr auto unitW = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    constexpr auto allOnes = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
}

namespace Matrix4
{
    constexpr auto identity = glm::mat4(1.0f);
}
