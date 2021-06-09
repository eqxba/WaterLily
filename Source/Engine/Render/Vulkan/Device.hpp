#pragma once

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

	VkDevice device;
	VkPhysicalDevice physicalDevice;

	Queues queues;
};