#pragma once

#include "Engine/Window.hpp"

#include <volk.h>

struct SwapchainSupportDetails
{
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class Swapchain
{
public:
	Swapchain(const Extent2D& requiredExtentInPixels);
	~Swapchain();

	void Recreate(const Extent2D& requiredExtentInPixels);

	VkSwapchainKHR swapchain;

	VkSurfaceFormatKHR surfaceFormat;
	VkExtent2D extent;

	std::vector<VkImage> images;
	std::vector<VkImageView> imageViews;

private:
	void Cleanup();
	
	SwapchainSupportDetails supportDetails;
};