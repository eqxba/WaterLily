#include "Engine/Engine.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/EventSystem.hpp"
#include "Engine/Systems/System.hpp"
#include "Engine/Systems/RenderSystem.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Systems/CameraSystem.hpp"

namespace EngineDetails
{
    static float GetDeltaSeconds(std::chrono::time_point<std::chrono::high_resolution_clock> start,
        std::chrono::time_point<std::chrono::high_resolution_clock> end)
    {
        std::chrono::duration<float> delta = end - start;
        return delta.count();
    }
}

Engine::Engine()
{
    eventSystem = std::make_unique<EventSystem>();
    window = std::make_unique<Window>(EngineConfig::windowWidth, EngineConfig::windowHeight, EngineConfig::engineName, *eventSystem);
    vulkanContext = std::make_unique<VulkanContext>(*window);

    eventSystem->Subscribe<ES::WindowResized>(this, &Engine::OnResize);

    CreateSystems();

    scene = std::make_unique<Scene>(*vulkanContext);
    eventSystem->Fire<ES::SceneOpened>({ *scene });
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
    using namespace std::chrono;

    time_point<high_resolution_clock> lastFrameTime = high_resolution_clock::now();

    while (!window->ShouldClose())
    {
        window->PollEvents();

        auto currentTime = high_resolution_clock::now();
        float deltaSeconds = EngineDetails::GetDeltaSeconds(lastFrameTime, currentTime);
        lastFrameTime = currentTime;

        std::ranges::for_each(systems, [=](const std::unique_ptr<System>& system) {
            system->Process(deltaSeconds);
        });

    	if (!renderingSuspended)
    	{
			renderSystem->Render();
    	}        
    }

    vulkanContext->GetDevice().WaitIdle();
}

void Engine::CreateSystems()
{
    auto renderSystemPtr = std::make_unique<RenderSystem>(*eventSystem, *vulkanContext);
    renderSystem = renderSystemPtr.get();

    systems.emplace_back(std::move(renderSystemPtr));
    systems.emplace_back(std::make_unique<CameraSystem>(*eventSystem));
}

void Engine::OnResize(const ES::WindowResized& event)
{
    renderingSuspended = event.newWidth == 0 || event.newHeight == 0;
}