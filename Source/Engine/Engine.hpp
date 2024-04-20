#pragma once

#include "Engine/Window.hpp"
#include "Engine/EngineConfig.hpp"

namespace ES
{
    struct WindowResized;   
}

class EventSystem;
class VulkanContext;
class Scene;
class System;
class RenderSystem;
class CameraSystem;

class Engine
{
public:   
    Engine();
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;   

    EventSystem& GetEventSystem() const;

    void Run();
	
private:
    void CreateSystems();

    void OnResize(const ES::WindowResized& event);
	
    std::unique_ptr<EventSystem> eventSystem;
    std::unique_ptr<Window> window;
    std::unique_ptr<VulkanContext> vulkanContext;
    std::unique_ptr<Scene> scene;

    RenderSystem* renderSystem;

    std::vector<std::unique_ptr<System>> systems;

    bool renderingSuspended = false;
};