#include "Engine/Components/CameraComponent.hpp"

namespace CameraComponentDetails
{
    static glm::quat GetQuat(glm::vec3 direction, glm::vec3 up)
    {
        direction = glm::normalize(direction);

        const glm::vec3 right = glm::normalize(glm::cross(direction, up));
        up = glm::normalize(glm::cross(right, direction));

        const glm::mat3 rotationMatrix = {
            right.x, up.x, -direction.x,
            right.y, up.y, -direction.y,
            right.z, up.z, -direction.z };

        return glm::normalize(glm::quat_cast(rotationMatrix));
    }
}

CameraComponent::CameraComponent()
    : rotationQuat{ CameraComponentDetails::GetQuat(-Vector3::unitZ, Vector3::unitY) }
{}

CameraComponent::CameraComponent(const glm::vec3 aPosition, const glm::vec3 direction, const glm::vec3 up, 
    const float aVerticalFov, const float aAspectRatio)
    : position{ aPosition }
    , verticalFov{ aVerticalFov }
    , aspectRatio{ aAspectRatio }
    , rotationQuat{ CameraComponentDetails::GetQuat(direction, up) }
{}
