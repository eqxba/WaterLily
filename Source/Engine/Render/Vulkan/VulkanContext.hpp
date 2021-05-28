#pragma once

#include <vulkan/vulkan.h>
#include <memory>

class Instance;

class VulkanContext
{
public:
	static void Create();
	static void Destroy();

	static std::unique_ptr<Instance> instance;
};