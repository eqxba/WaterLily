#include "Engine/Systems/CameraSystem.hpp"

#include "Engine/Components/CameraComponent.hpp"
#include "Engine/EventSystem.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "GLFW/glfw3.h"

namespace CameraSystemDetails
{
    static const std::unordered_set movementKeys = { Key::eW, Key::eA, Key::eS, Key::eD, Key::eQ, Key::eE };
    static constexpr std::array speedKeys = { Key::e1, Key::e2, Key::e3, Key::e4, Key::e5 };
    static constexpr float cameraSpeed = 2.0f;

    static glm::mat4 CalculateViewMatrix(const CameraComponent& camera)
    {
        return glm::lookAt(camera.position, camera.position + camera.direction, camera.up);
    }
}

CameraSystem::CameraSystem(EventSystem& aEventSystem)
    : eventSystem{ aEventSystem }
{
    eventSystem.Subscribe<ES::SceneOpened>(this, &CameraSystem::OnSceneOpen);
    eventSystem.Subscribe<ES::KeyInput>(this, &CameraSystem::OnKeyInput);
}

CameraSystem::~CameraSystem()
{
    eventSystem.Unsubscribe<ES::SceneOpened>(this);
    eventSystem.Unsubscribe<ES::KeyInput>(this);
}

void CameraSystem::Process(float deltaSeconds)
{
    using namespace CameraSystemDetails;

    if (mainCamera)
    {
        glm::vec3 moveDirection(0.0f);

        if (pressedMovementKeys.contains(Key::eW))
        {
            moveDirection += mainCamera->direction;
        }
        if (pressedMovementKeys.contains(Key::eA))
        {
            moveDirection -= glm::normalize(glm::cross(mainCamera->direction, mainCamera->up));
        }
        if (pressedMovementKeys.contains(Key::eS))
        {
            moveDirection -= mainCamera->direction;
        }
        if (pressedMovementKeys.contains(Key::eD))
        {
            moveDirection += glm::normalize(glm::cross(mainCamera->direction, mainCamera->up));
        }
        if (pressedMovementKeys.contains(Key::eQ))
        {
            moveDirection -= mainCamera->up;
        }
        if (pressedMovementKeys.contains(Key::eE))
        {
            moveDirection += mainCamera->up;
        }

        mainCamera->position += deltaSeconds * cameraSpeed * speedMultiplier * moveDirection;
        mainCamera->view = CalculateViewMatrix(*mainCamera);
    }
}

void CameraSystem::OnSceneOpen(const ES::SceneOpened& event)
{
    mainCamera = &event.scene.GetCamera();
    mainCamera->direction = glm::normalize(mainCamera->direction);
    mainCamera->up = glm::normalize(mainCamera->up);
}

void CameraSystem::OnKeyInput(const ES::KeyInput& event)
{
    using namespace CameraSystemDetails;

    if (event.action == KeyAction::eRepeat)
    {
        return;
    }

    if (movementKeys.contains(event.key))
    {
        if (event.action == KeyAction::ePress)
        {
            pressedMovementKeys.emplace(event.key);
        }
        else if (event.action == KeyAction::eRelease)
        {
            pressedMovementKeys.erase(event.key);
        }
    }

    if (const auto it = std::ranges::find(speedKeys, event.key); it != speedKeys.end())
    {
        speedMultiplier = static_cast<float>(std::distance(speedKeys.begin(), it) + 1);
    }
}