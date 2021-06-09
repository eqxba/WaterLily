#include "Engine/Engine.hpp"

#include "Engine/EngineConfig.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Systems/RenderSystem.hpp"

std::unique_ptr<Window> Engine::window;
std::unique_ptr<RenderSystem> Engine::renderSystem;

void Engine::Create()
{
    window = std::make_unique<Window>(EngineConfig::windowWidth, EngineConfig::windowHeight, EngineConfig::engineName);

    VulkanContext::Create(*window);

    renderSystem = std::make_unique<RenderSystem>();
}

void Engine::Run()
{
    while (!window->ShouldClose())
    {
        window->PollEvents();
        renderSystem->Render();
    }

    VulkanContext::device->WaitIdle();
}

void Engine::Destroy()
{
    renderSystem.reset();
    window.reset();
}