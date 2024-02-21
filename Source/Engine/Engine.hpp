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
class RenderSystem;

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
    void OnResize(const ES::WindowResized& event);
	
    std::unique_ptr<EventSystem> eventSystem = std::make_unique<EventSystem>();
    std::unique_ptr<Window> window = std::make_unique<Window>(EngineConfig::windowWidth, EngineConfig::windowHeight, EngineConfig::engineName, *eventSystem);
    std::unique_ptr<VulkanContext> vulkanContext = std::make_unique<VulkanContext>(*window);
    std::unique_ptr<Scene> scene = std::make_unique<Scene>(*vulkanContext);
    std::unique_ptr<RenderSystem> renderSystem = std::make_unique<RenderSystem>(*scene, *eventSystem, *vulkanContext);

    bool renderingSuspended = false;
};