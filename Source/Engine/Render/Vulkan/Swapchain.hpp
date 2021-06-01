#pragma once

#include <volk.h>

class Window;

struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class Swapchain
{
public:
	Swapchain(const Window& window);
	~Swapchain();

	VkSwapchainKHR swapchain;

	VkFormat format;
	VkExtent2D extent;

	std::vector<VkImage> images;
	std::vector<VkImageView> imageViews;
};