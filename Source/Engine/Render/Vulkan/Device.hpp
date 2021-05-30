#pragma once

#include <volk.h>

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentationFamily;

	bool IsComplete() const
	{
		return graphicsFamily.has_value() && presentationFamily.has_value();
	}
};

struct Queues
{
	VkQueue graphics;
	VkQueue presentation;
};

class Device
{
public:
	Device();
	~Device();

	VkDevice device;
	VkPhysicalDevice physicalDevice;

	Queues queues;
};