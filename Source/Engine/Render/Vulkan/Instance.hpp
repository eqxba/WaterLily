#pragma once

#include <volk.h>

class Instance
{
public:
	Instance();
	~Instance();

	VkInstance instance;

private:	
	VkDebugUtilsMessengerEXT debugMessenger;
};