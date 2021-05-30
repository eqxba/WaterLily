#include "Engine/Engine.hpp"

#include "Engine/EngineConfig.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Window.hpp"

std::unique_ptr<Window> Engine::window;

void Engine::Create()
{
    window = std::make_unique<Window>(EngineConfig::windowWidth, EngineConfig::windowHeight, EngineConfig::engineName);

    VulkanContext::Create(*window);
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