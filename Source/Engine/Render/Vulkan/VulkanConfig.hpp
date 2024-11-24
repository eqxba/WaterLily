#pragma once

#include <volk.h>

namespace VulkanConfig
{
	constexpr uint32_t apiVersion = VK_API_VERSION_1_3;

#ifdef NDEBUG
	constexpr bool useValidation = false;
#else
	constexpr bool useValidation = true;
#endif

	const std::vector<const char*> requiredDeviceExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef PLATFORM_MAC
        "VK_KHR_portability_subset"
#endif
	};

	constexpr int maxFramesInFlight = 2;

	constexpr VkFormat depthImageFormat = VK_FORMAT_D32_SFLOAT;
}
