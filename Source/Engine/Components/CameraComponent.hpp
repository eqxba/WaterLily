#pragma once

#include "macros.hpp"
#include "Utils/Constants.hpp"

DISABLE_WARNINGS_BEGIN
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
DISABLE_WARNINGS_END

class CameraComponent
{
public:
    CameraComponent();
    CameraComponent(glm::vec3 position, glm::vec3 direction, glm::vec3 up, float verticalFov, float aspectRatio);

    glm::vec3 GetPosition() const
    {
        return position;
    }

    void SetPosition(glm::vec3 aPosition)
    {
        position = aPosition;
    }

    float GetVerticalFov() const
    {
        return verticalFov;
    }

    void SetVerticalFov(const float aVerticalFov)
    {
        verticalFov = aVerticalFov;
    }

    float GetAspectRatio() const
    {
        return aspectRatio;
    }

    void SetAspectRatio(const float aAspectRatio)
    {
        aspectRatio = aAspectRatio;
    }

    glm::quat GetRotationQuat() const
    {
        return rotationQuat;
    }

    void SetRotationQuat(const glm::quat aRotationQuat)
    {
        rotationQuat = aRotationQuat;
    }

    glm::vec3 GetDirection() const
    {
        return rotationQuat * -Vector3::unitZ;
    }

    glm::vec3 GetRight() const
    {
        return rotationQuat * Vector3::unitX;
    }

    glm::vec3 GetUp() const
    {
        return rotationQuat * Vector3::unitY;
    }

    glm::mat4 GetViewMatrix() const
    {
        return glm::lookAt(position, position + GetDirection(), GetUp());
    }

    glm::mat4 GetProjectionMatrix() const
    {
        glm::mat4 projection = glm::perspective(verticalFov, aspectRatio, 0.1f, 100.0f);
        projection[1][1] *= -1;
        return projection;
    }

private:
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 5.0f);

    float verticalFov = glm::radians(45.0f);
    float aspectRatio = 1.0f;

    glm::quat rotationQuat = {};
};
