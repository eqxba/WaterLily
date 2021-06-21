#pragma once

#include "Engine/Window.hpp"

namespace ES
{
    struct WindowResized;   
}

class RenderSystem;
class EventSystem;

class Engine
{
public:
    static void Create();
    static void Run();
    static void Destroy();

    static EventSystem* GetEventSystem()
    {
        return eventSystem.get();
    }
	
private:
    static void OnResize(const ES::WindowResized& event);
	
    static std::unique_ptr<Window> window;
    static std::unique_ptr<RenderSystem> renderSystem;
    static std::unique_ptr<EventSystem> eventSystem;

    static bool renderingSuspended;
};