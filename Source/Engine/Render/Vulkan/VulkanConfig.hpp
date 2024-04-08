#pragma once

#include <volk.h>

namespace VulkanConfig
{
#ifdef NDEBUG
	constexpr bool useValidation = false;
#else
	constexpr bool useValidation = true;
#endif

	const std::vector<const char*> requiredDeviceExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	constexpr int maxFramesInFlight = 2;

	constexpr VkFormat depthImageFormat = VK_FORMAT_D32_SFLOAT;
}
