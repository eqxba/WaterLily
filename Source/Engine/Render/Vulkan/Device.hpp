#pragma once

#include <volk.h>

using DeviceCommands = std::function<void(VkCommandBuffer)>;

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

enum class CommandBufferType
{
    eOneTime,
    eLongLived
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
};