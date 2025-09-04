#pragma once

#include "Utils/Constants.hpp"

struct Extent2D
{
    int width = 0;
    int height = 0;
};

enum class InputMode
{
    eEngine,
    eUi,
};

struct Sphere
{
    glm::vec3 center = Vector3::zero;
    float radius = 0.0f;
};
