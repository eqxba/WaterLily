#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Engine/Window.hpp"

#define VOLK_IMPLEMENTATION
#include <volk.h>

static bool volkInitialized = false;

VulkanContext::VulkanContext(const Window& window)
{
	if (!volkInitialized) 
	{
		const VkResult volkInitializeResult = volkInitialize();
		Assert(volkInitializeResult == VK_SUCCESS);
		volkInitialized = true;
	}

	instance = std::make_unique<Instance>();
	surface = std::make_unique<Surface>(window, *this);
	device = std::make_unique<Device>(*this);
	swapchain = std::make_unique<Swapchain>(window.GetExtentInPixels(), *this);

	memoryManager = std::make_unique<MemoryManager>(*this);
	shaderManager = std::make_unique<ShaderManager>(*this);
}