#pragma once

#include "Utils/Helpers.hpp"

#include <glm/glm.hpp>

struct CameraComponent
{
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 5.0f);
    glm::vec3 direction = -Vector3::unitZ;
    glm::vec3 up = Vector3::unitY;

    float verticalFov = glm::radians(45.0f);

    glm::mat4 view = {};
    glm::mat4 projection = {};
};
