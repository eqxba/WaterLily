#pragma once

#include "Engine/Systems/System.hpp"
#include "Engine/InputHelpers.hpp"

#include <glm/glm.hpp>

namespace ES
{
    struct SceneOpened;
    struct KeyInput;
    struct MouseMoved;
}

struct CameraComponent;

class EventSystem;

class CameraSystem : public System
{
public:
    CameraSystem(EventSystem& aEventSystem);
    ~CameraSystem() override;

    CameraSystem(const CameraSystem&) = delete;
    CameraSystem& operator=(const CameraSystem&) = delete;

    CameraSystem(CameraSystem&&) = delete;
    CameraSystem& operator=(CameraSystem&&) = delete;

    void Process(float deltaSeconds) override;

private:
    void OnSceneOpen(const ES::SceneOpened& event);
    void OnKeyInput(const ES::KeyInput& event);
    void OnMouseMoved(const ES::MouseMoved& event);

    EventSystem& eventSystem;

    CameraComponent* mainCamera = nullptr;

    std::unordered_set<Key> pressedMovementKeys;

    float speedMultiplier = 1.0f;

    std::optional<glm::vec2> lastMousePosition = {};
    glm::vec2 mouseDelta = {};
};