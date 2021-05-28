#include "Engine/Engine.hpp"

#include "Engine/EngineConfig.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Window.hpp"

std::unique_ptr<Window> Engine::window;

void Engine::Create()
{
    // TODO: Move resolution to config
    window = std::make_unique<Window>(1920, 1080, EngineConfig::engineName);

    VulkanContext::Create();
}

void Engine::Run()
{
    while (!window->ShouldClose())
    {
        window->PollEvents();
    }
}

void Engine::Destroy()
{

}