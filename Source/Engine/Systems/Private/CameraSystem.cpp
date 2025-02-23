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
            const glm::quat yawQuat = glm::angleAxis(-mouseDelta.x, Vector3::unitY);
            const glm::quat pitchQuat = glm::angleAxis(mouseDelta.y, camera.GetRight());
            const glm::quat deltaQuat = yawQuat * pitchQuat;

            camera.SetRotationQuat(glm::normalize(deltaQuat * camera.GetRotationQuat()));

            mouseDelta = Vector2::zero;
        }
    }
}

CameraSystem::CameraSystem(const Extent2D windowExtent, CursorMode aCursorMode, EventSystem& aEventSystem)
    : eventSystem{ aEventSystem }
    , mainCameraAspectRatio{ static_cast<float>(windowExtent.width) / static_cast<float>(windowExtent.height) }
    , cursorMode{ aCursorMode }
{
    eventSystem.Subscribe<ES::SceneOpened>(this, &CameraSystem::OnSceneOpen);
    eventSystem.Subscribe<ES::WindowResized>(this, &CameraSystem::OnResize);
    eventSystem.Subscribe<ES::WindowRecreated>(this, &CameraSystem::OnWindowRecreated);
    eventSystem.Subscribe<ES::KeyInput>(this, &CameraSystem::OnKeyInput);
    eventSystem.Subscribe<ES::MouseMoved>(this, &CameraSystem::OnMouseMoved);
    eventSystem.Subscribe<ES::MouseWheelScrolled>(this, &CameraSystem::OnMouseWheelScrolled);
    eventSystem.Subscribe<ES::BeforeCursorModeUpdated>(this, &CameraSystem::OnBeforeCursorModeUpdated);
}

CameraSystem::~CameraSystem()
{
    // TODO: UnsubscribeAll implementation
    eventSystem.Unsubscribe<ES::SceneOpened>(this);
    eventSystem.Unsubscribe<ES::WindowResized>(this);
    eventSystem.Unsubscribe<ES::WindowRecreated>(this);
    eventSystem.Unsubscribe<ES::KeyInput>(this);
    eventSystem.Unsubscribe<ES::MouseMoved>(this);
    eventSystem.Unsubscribe<ES::MouseWheelScrolled>(this);
    eventSystem.Unsubscribe<ES::BeforeCursorModeUpdated>(this);
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
    const glm::vec3 cameraRight = camera.GetRight();

    glm::vec3 moveDirection(0.0f);

    if (pressedMovementKeys.contains(Key::eW))
    {
        moveDirection += cameraDirection;
    }
    if (pressedMovementKeys.contains(Key::eA))
    {
        moveDirection -= cameraRight;
    }
    if (pressedMovementKeys.contains(Key::eS))
    {
        moveDirection -= cameraDirection;
    }
    if (pressedMovementKeys.contains(Key::eD))
    {
        moveDirection += cameraRight;
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

void CameraSystem::UpdateAspectRatio(const Extent2D newExtent)
{
    if (newExtent.width == 0 || newExtent.height == 0)
    {
        return;
    }

    mainCameraAspectRatio = static_cast<float>(newExtent.width) / static_cast<float>(newExtent.height);

    if (mainCamera)
    {
        mainCamera->SetAspectRatio(mainCameraAspectRatio);
    }
}

void CameraSystem::OnSceneOpen(const ES::SceneOpened& event)
{
    mainCamera = &event.scene.GetCamera();
    mainCamera->SetAspectRatio(mainCameraAspectRatio);
}

void CameraSystem::OnResize(const ES::WindowResized& event)
{
    UpdateAspectRatio(event.newExtent);
}

void CameraSystem::OnWindowRecreated(const ES::WindowRecreated& event)
{
    lastMousePosition.reset();
    UpdateAspectRatio(event.window->GetExtentInPixels());
}

void CameraSystem::OnKeyInput(const ES::KeyInput& event)
{
    using namespace CameraSystemDetails;

    if (event.action == KeyAction::eRepeat || cursorMode == CursorMode::eEnabled)
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
    if (cursorMode == CursorMode::eEnabled)
    {
        return;
    }

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

    if (cursorMode == CursorMode::eDisabled && mainCamera)
    {
        const float deltaFov = - static_cast<float>(event.offset.y) * mouseScrollSensitivity;
        const float newFov = std::clamp(mainCamera->GetVerticalFov() + deltaFov, minVerticalFov, maxVerticalFov);

        mainCamera->SetVerticalFov(newFov);
    }
}

void CameraSystem::OnBeforeCursorModeUpdated(const ES::BeforeCursorModeUpdated& event)
{
    lastMousePosition.reset();

    cursorMode = event.newCursorMode;

    if (cursorMode == CursorMode::eEnabled)
    {
        pressedMovementKeys.clear();
    }
}