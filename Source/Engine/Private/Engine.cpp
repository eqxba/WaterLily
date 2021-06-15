#include "Engine/Engine.hpp"

#include "Engine/EngineConfig.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Systems/RenderSystem.hpp"

std::unique_ptr<Window> Engine::window;
std::unique_ptr<RenderSystem> Engine::renderSystem;
bool Engine::renderingSuspended = false;

void Engine::Create()
{
    window = std::make_unique<Window>(EngineConfig::windowWidth, EngineConfig::windowHeight, EngineConfig::engineName);

    VulkanContext::Create(*window);

    // TODO: Subscribe to events
	
    renderSystem = std::make_unique<RenderSystem>();
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

    VulkanContext::device->WaitIdle();
}

void Engine::Destroy()
{
    renderSystem.reset();
    window.reset();
}

void Engine::OnResize(const Extent2D& newSize)
{
    VulkanContext::device->WaitIdle();

    renderingSuspended = newSize.width == 0 || newSize.height == 0;

	if (!renderingSuspended)
	{
        VulkanContext::swapchain->Recreate(newSize);
		
        renderSystem->OnResize();
	}
}