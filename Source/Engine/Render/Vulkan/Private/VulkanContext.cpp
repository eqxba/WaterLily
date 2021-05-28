#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Engine/Render/Vulkan/Instance.hpp"

std::unique_ptr<Instance> VulkanContext::instance;

void VulkanContext::Create()
{
	instance = std::make_unique<Instance>();
}

void VulkanContext::Destroy()
{
	instance.reset();
}