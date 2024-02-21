#pragma once

#include "Engine/Window.hpp"

#include <volk.h>

class VulkanContext;

struct SwapchainSupportDetails
{
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class Swapchain
{
public:
	Swapchain(const Extent2D& requiredExtentInPixels, const VulkanContext& aVulkanContext);
	~Swapchain();

	Swapchain(const Swapchain&) = delete;
	Swapchain& operator=(const Swapchain&) = delete;

	Swapchain(Swapchain&&) = delete;
	Swapchain& operator=(Swapchain&&) = delete;

	void Recreate(const Extent2D& requiredExtentInPixels);

	VkSwapchainKHR GetVkSwapchainKHR() const
	{
		return swapchain;
	}

	VkSurfaceFormatKHR GetSurfaceFormat() const
	{
		return surfaceFormat;
	}

	VkExtent2D GetExtent() const
	{
		return extent;
	}

	const std::vector<VkImage>& GetImages() const
	{
		return images;
	}

	const std::vector<VkImageView>& GetImageViews() const
	{
		return imageViews;
	}

private:
	void Create(const Extent2D& requiredExtentInPixels);
	void Cleanup();

	const VulkanContext& vulkanContext;
	
	SwapchainSupportDetails supportDetails;

	VkSwapchainKHR swapchain;

	VkSurfaceFormatKHR surfaceFormat; 
	VkExtent2D extent;

	std::vector<VkImage> images;
	std::vector<VkImageView> imageViews;
};