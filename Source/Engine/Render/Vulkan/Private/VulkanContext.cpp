#include "Engine/Render/Vulkan/VulkanContext.hpp"

#define VOLK_IMPLEMENTATION
#include <volk.h>

std::unique_ptr<Instance> VulkanContext::instance;
std::unique_ptr<Surface> VulkanContext::surface;
std::unique_ptr<Device> VulkanContext::device;
std::unique_ptr<Swapchain> VulkanContext::swapchain;

std::unique_ptr<ShaderManager> VulkanContext::shaderManager;

void VulkanContext::Create(const Window& window)
{
	const VkResult volkInitializeResult = volkInitialize();
	Assert(volkInitializeResult == VK_SUCCESS);
	
	instance = std::make_unique<Instance>();
	surface = std::make_unique<Surface>(window);
	device = std::make_unique<Device>();
	swapchain = std::make_unique<Swapchain>(window.GetExtentInPixels());

	shaderManager = std::make_unique<ShaderManager>();
}

void VulkanContext::Destroy()
{
	shaderManager.reset();
	
	swapchain.reset();
	device.reset();
	surface.reset();
	instance.reset();
}