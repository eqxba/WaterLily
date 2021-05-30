#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Engine/Window.hpp"
#include "Engine/Render/Vulkan/Instance.hpp"
#include "Engine/Render/Vulkan/Surface.hpp"
#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Swapchain.hpp"

#define VOLK_IMPLEMENTATION
#include <volk.h>

std::unique_ptr<Instance> VulkanContext::instance;
std::unique_ptr<Surface> VulkanContext::surface;
std::unique_ptr<Device> VulkanContext::device;
std::unique_ptr<Swapchain> VulkanContext::swapchain;

void VulkanContext::Create(const Window& window)
{
	const VkResult volkInitializeResult = volkInitialize();
	Assert(volkInitializeResult == VK_SUCCESS);
	
	instance = std::make_unique<Instance>();
	surface = std::make_unique<Surface>(window);
	device = std::make_unique<Device>();
	swapchain = std::make_unique<Swapchain>(window);
}

void VulkanContext::Destroy()
{
	swapchain.reset();
	device.reset();
	surface.reset();
	instance.reset();
}