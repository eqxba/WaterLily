#pragma once

#include "Engine/Systems/System.hpp"
#include "Utils/DataStructures.hpp"
#include "Engine/InputUtils.hpp"

DISABLE_WARNINGS_BEGIN
#include <glm/glm.hpp>
DISABLE_WARNINGS_END

namespace ES
{
    struct SceneOpened;
    struct WindowResized;
    struct WindowRecreated;
    struct KeyInput;
    struct MouseMoved;
    struct MouseWheelScrolled;
    struct BeforeInputModeUpdated;
}

class CameraComponent;

class EventSystem;

class CameraSystem : public System
{
public:
    CameraSystem(Extent2D windowExtent, InputMode cursorMode, EventSystem& aEventSystem);
    ~CameraSystem() override;

    CameraSystem(const CameraSystem&) = delete;
    CameraSystem& operator=(const CameraSystem&) = delete;

    CameraSystem(CameraSystem&&) = delete;
    CameraSystem& operator=(CameraSystem&&) = delete;

    void Process(float deltaSeconds) override;

private:
    void UpdatePosition(CameraComponent& camera, float deltaSeconds) const;
    void UpdateAspectRatio(Extent2D newExtent);

    void OnSceneOpen(const ES::SceneOpened& event);
    void OnResize(const ES::WindowResized& event);
    void OnWindowRecreated(const ES::WindowRecreated& event);
    void OnKeyInput(const ES::KeyInput& event);
    void OnMouseMoved(const ES::MouseMoved& event);
    void OnMouseWheelScrolled(const ES::MouseWheelScrolled& event);
    void OnBeforeInputModeUpdated(const ES::BeforeInputModeUpdated& event);

    EventSystem& eventSystem;

    float mainCameraAspectRatio = 1.0f;

    CameraComponent* mainCamera = nullptr;

    std::unordered_set<Key> pressedMovementKeys;

    float speedMultiplier = 1.0f;

    std::optional<glm::vec2> lastMousePosition = {};
    glm::vec2 mouseDelta = {};

    InputMode inputMode = InputMode::eEngine;
};