#pragma once

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include <volk.h>

struct QueueFamilyIndices
{
	uint32_t graphicsFamily;
	uint32_t presentFamily;
};

struct Queues
{	
	VkQueue graphics;
	VkQueue present;
	QueueFamilyIndices familyIndices;
};

class Device
{
public:
	Device();
	~Device();

	void WaitIdle() const;

	VkCommandPool GetCommandPool(CommandBufferType type);
	void ExecuteOneTimeCommandBuffer(DeviceCommands commands);

	VkDevice device;
	VkPhysicalDevice physicalDevice;

	Queues queues;

private:
	std::unordered_map<CommandBufferType, VkCommandPool> commandPools;

	CommandBufferSync oneTimeCommandBufferSync;
};