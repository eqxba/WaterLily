#include "Engine/Systems/CameraSystem.hpp"

#include "Engine/Components/CameraComponent.hpp"
#include "Engine/EventSystem.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace CameraSystemDetails
{
    static const std::unordered_set movementKeys = { Key::eW, Key::eA, Key::eS, Key::eD, Key::eQ, Key::eE };
    static constexpr std::array speedKeys = { Key::e1, Key::e2, Key::e3, Key::e4, Key::e5 };
    static constexpr float cameraSpeed = 2.0f;
    static constexpr float mouseSensitivity = 0.001f;
    static constexpr float mouseScrollSensitivity = 0.02f;
    static constexpr float minVerticalFov = glm::radians(1.0f);
    static constexpr float maxVerticalFov = glm::radians(45.0f);

    // Resets mouseDelta to (0.0f, 0.0f)
    static void UpdateRotation(CameraComponent& camera, glm::vec2& mouseDelta)
    {
        if (mouseDelta.x != 0.0f || mouseDelta.y != 0.0f)
        {
            const glm::vec3 cameraRight = glm::cross(camera.GetDirection(), camera.GetUp());

            const glm::quat yawQuat = glm::angleAxis(-mouseDelta.x, Vector3::unitY);
            const glm::quat pitchQuat = glm::angleAxis(mouseDelta.y, cameraRight);
            const glm::quat deltaQuat = glm::normalize(pitchQuat * yawQuat);

            camera.SetRotationQuat(glm::normalize(deltaQuat * camera.GetRotationQuat()));

            mouseDelta = Vector2::zero;
        }
    }
}

CameraSystem::CameraSystem(const Extent2D windowExtent, EventSystem& aEventSystem)
    : eventSystem{ aEventSystem }
    , mainCameraAspectRatio{ static_cast<float>(windowExtent.width) / static_cast<float>(windowExtent.height) }
{
    eventSystem.Subscribe<ES::SceneOpened>(this, &CameraSystem::OnSceneOpen);
    eventSystem.Subscribe<ES::WindowResized>(this, &CameraSystem::OnResize);
    eventSystem.Subscribe<ES::KeyInput>(this, &CameraSystem::OnKeyInput);
    eventSystem.Subscribe<ES::MouseMoved>(this, &CameraSystem::OnMouseMoved);
    eventSystem.Subscribe<ES::MouseWheelScrolled>(this, &CameraSystem::OnMouseWheelScrolled);
}

CameraSystem::~CameraSystem()
{
    // TODO: UnsubscribeAll implementation
    eventSystem.Unsubscribe<ES::SceneOpened>(this);
    eventSystem.Unsubscribe<ES::WindowResized>(this);
    eventSystem.Unsubscribe<ES::KeyInput>(this);
    eventSystem.Unsubscribe<ES::MouseMoved>(this);
    eventSystem.Unsubscribe<ES::MouseWheelScrolled>(this);
}

void CameraSystem::Process(const float deltaSeconds)
{
    using namespace CameraSystemDetails;

    if (mainCamera)
    {
        UpdateRotation(*mainCamera, mouseDelta);
        UpdatePosition(*mainCamera, deltaSeconds);
    }
}

void CameraSystem::UpdatePosition(CameraComponent& camera, const float deltaSeconds) const
{
    using namespace CameraSystemDetails;

    const glm::vec3 cameraDirection = camera.GetDirection();
    const glm::vec3 cameraUp = camera.GetUp();

    glm::vec3 moveDirection(0.0f);

    if (pressedMovementKeys.contains(Key::eW))
    {
        moveDirection += cameraDirection;
    }
    if (pressedMovementKeys.contains(Key::eA))
    {
        moveDirection -= glm::normalize(glm::cross(cameraDirection, cameraUp));
    }
    if (pressedMovementKeys.contains(Key::eS))
    {
        moveDirection -= cameraDirection;
    }
    if (pressedMovementKeys.contains(Key::eD))
    {
        moveDirection += glm::normalize(glm::cross(cameraDirection, cameraUp));
    }
    if (pressedMovementKeys.contains(Key::eQ))
    {
        moveDirection -= cameraUp;
    }
    if (pressedMovementKeys.contains(Key::eE))
    {
        moveDirection += cameraUp;
    }

    camera.SetPosition(camera.GetPosition() + deltaSeconds * cameraSpeed * speedMultiplier * moveDirection);
}

void CameraSystem::OnSceneOpen(const ES::SceneOpened& event)
{
    mainCamera = &event.scene.GetCamera();
    mainCamera->SetAspectRatio(mainCameraAspectRatio);
}

void CameraSystem::OnResize(const ES::WindowResized& event)
{
    mainCameraAspectRatio = static_cast<float>(event.newExtent.width) / static_cast<float>(event.newExtent.height);

    if (mainCamera)
    {
        mainCamera->SetAspectRatio(mainCameraAspectRatio);
    }
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

void CameraSystem::OnMouseMoved(const ES::MouseMoved& event)
{
    if (lastMousePosition)
    {
        mouseDelta = event.newPosition - lastMousePosition.value();
        mouseDelta *= CameraSystemDetails::mouseSensitivity;
        mouseDelta.y *= -1; // Flip y
    }

    lastMousePosition = event.newPosition;
}

void CameraSystem::OnMouseWheelScrolled(const ES::MouseWheelScrolled& event)
{
    using namespace CameraSystemDetails;

    if (mainCamera)
    {
        const float deltaFov = - static_cast<float>(event.offset.y) * mouseScrollSensitivity;
        const float newFov = std::clamp(mainCamera->GetVerticalFov() + deltaFov, minVerticalFov, maxVerticalFov);

        mainCamera->SetVerticalFov(newFov);
    }
}
