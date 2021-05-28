#pragma once

#include <vulkan/vulkan.h>

class Instance
{
public:
	Instance();
	~Instance();

	VkInstance instance;

private:	
	VkDebugUtilsMessengerEXT debugMessenger;
};