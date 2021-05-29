#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Engine/Render/Vulkan/Instance.hpp"

#define VOLK_IMPLEMENTATION
#include <volk.h>

std::unique_ptr<Instance> VulkanContext::instance;

void VulkanContext::Create()
{
	const VkResult volkInitializeResult = volkInitialize();
	Assert(volkInitializeResult == VK_SUCCESS);
	
	instance = std::make_unique<Instance>();
}

void VulkanContext::Destroy()
{
	instance.reset();
}