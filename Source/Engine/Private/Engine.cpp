#include "Engine/Engine.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/EventSystem.hpp"
#include "Engine/Systems/RenderSystem.hpp"
#include "Engine/Scene/Scene.hpp"

Engine::Engine()
{
    eventSystem = std::make_unique<EventSystem>();
    window = std::make_unique<Window>(EngineConfig::windowWidth, EngineConfig::windowHeight, EngineConfig::engineName, *eventSystem);
    vulkanContext = std::make_unique<VulkanContext>(*window);
    scene = std::make_unique<Scene>(*vulkanContext);
    renderSystem = std::make_unique<RenderSystem>(*scene, *eventSystem, *vulkanContext);

    eventSystem->Subscribe<ES::WindowResized>(this, &Engine::OnResize);
}

Engine::~Engine()
{
    eventSystem->Unsubscribe<ES::WindowResized>(this);
}

EventSystem& Engine::GetEventSystem() const
{
    return *eventSystem;
}

void Engine::Run()
{
    while (!window->ShouldClose())
    {
        window->PollEvents();

    	if (!renderingSuspended)
    	{
			renderSystem->Render();
    	}        
    }

    vulkanContext->GetDevice().WaitIdle();
}

void Engine::OnResize(const ES::WindowResized& event)
{
    renderingSuspended = event.newWidth == 0 || event.newHeight == 0;
}