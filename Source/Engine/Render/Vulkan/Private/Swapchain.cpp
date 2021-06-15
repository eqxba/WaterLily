#include "Engine/Render/Vulkan/Swapchain.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace SwapchainDetails
{
	static VkSurfaceCapabilitiesKHR GetSurfaceCapabilities(VkPhysicalDevice device)
	{
		VkSurfaceCapabilitiesKHR capabilities;		
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, VulkanContext::surface->surface, &capabilities);
		return capabilities;
	}
	
	static SwapchainSupportDetails GetSwapchainSupportDetails(VkPhysicalDevice device)
	{
		SwapchainSupportDetails details;
		
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, VulkanContext::surface->surface, &formatCount, nullptr);
		Assert(formatCount != 0);
	
		details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, VulkanContext::surface->surface, &formatCount,
			details.formats.data());

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, VulkanContext::surface->surface, &presentModeCount, nullptr);
		Assert(presentModeCount != 0);
				
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, VulkanContext::surface->surface, &presentModeCount,
			details.presentModes.data());
		
		return details;
	}

	static VkSurfaceFormatKHR SelectSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		const auto isPreferredSurfaceFormat = [](const auto& surfaceFormat)
		{
			return surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
				surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		};

		const auto it = std::ranges::find_if(availableFormats, isPreferredSurfaceFormat);

		if (it != availableFormats.end())
		{
			return *it;
		}
		
		return availableFormats.front();
	}

	static VkPresentModeKHR SelectPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		const auto isPreferredPresentMode = [](const auto& presentMode)
		{
			return presentMode == VK_PRESENT_MODE_MAILBOX_KHR;
		};

		const auto it = std::ranges::find_if(availablePresentModes, isPreferredPresentMode);

		if (it != availablePresentModes.end())
		{
			return *it;
		}
		
		// This one is guaranteed to be supported
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	static VkExtent2D SelectExtent(const VkSurfaceCapabilitiesKHR& capabilities, const Extent2D& requiredExtentInPixels)
	{
		// currentExtent is the current width and height of the surface, or the special value (0xFFFFFFFF, 0xFFFFFFFF)
		// indicating that the surface size will be determined by the extent of a swapchain targeting the surface.
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}
		else
		{
			const VkExtent2D actualExtent =
			{
				std::clamp(static_cast<uint32_t>(requiredExtentInPixels.width),
					capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
				std::clamp(static_cast<uint32_t>(requiredExtentInPixels.height),
					capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
			};
			
			return actualExtent;
		}
	}

	static uint32_t SelectImageCount(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		uint32_t imageCount = capabilities.minImageCount + 1;

		// A value of 0 means that there is no limit on the number of images,
		// though there may be limits related to the total amount of memory used by presentable images.
		if (capabilities.maxImageCount != 0 && imageCount > capabilities.maxImageCount)
		{
			imageCount = capabilities.maxImageCount;
		}

		return imageCount;
	}

	static VkSwapchainKHR CreateSwapchain(const SwapchainSupportDetails& supportDetails, 
		const VkSurfaceCapabilitiesKHR& capabilities, const VkSurfaceFormatKHR surfaceFormat, const VkExtent2D extent)
	{
		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = VulkanContext::surface->surface;
		createInfo.minImageCount = SelectImageCount(capabilities);
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		// For now i'm going to render directly to swapchain images w/out anything like post-processing.
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		const QueueFamilyIndices& familyIndices = VulkanContext::device->queues.familyIndices;
		const uint32_t queueFamilyIndices[] = { familyIndices.graphicsFamily, familyIndices.presentFamily };

		if (familyIndices.graphicsFamily != familyIndices.presentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}

		createInfo.preTransform = capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = SelectPresentMode(supportDetails.presentModes);
		createInfo.clipped = VK_TRUE;
		// TODO: Create swapchain with the use of the old one
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		VkSwapchainKHR swapchain;
		const VkResult result = vkCreateSwapchainKHR(VulkanContext::device->device, &createInfo, nullptr, &swapchain);
		Assert(result == VK_SUCCESS);

		return swapchain;
	}

	static std::vector<VkImage> GetSwapchainImages(VkSwapchainKHR swapchain)
	{		
		uint32_t imageCount;
		vkGetSwapchainImagesKHR(VulkanContext::device->device, swapchain, &imageCount, nullptr);

		std::vector<VkImage> images(imageCount);
		vkGetSwapchainImagesKHR(VulkanContext::device->device, swapchain, &imageCount, images.data());

		return images;
	}

	static std::vector<VkImageView> CreateImageViews(const std::vector<VkImage>& images, VkFormat format)
	{
		std::vector<VkImageView> imageViews;
		imageViews.reserve(images.size());
		
		for (const auto image : images)
		{
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = image;
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = format;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			VkImageView imageView;
			const VkResult result = vkCreateImageView(VulkanContext::device->device, &createInfo, nullptr, &imageView);
			Assert(result == VK_SUCCESS);
			
			imageViews.emplace_back(imageView);
		}

		return imageViews;
	}
}

Swapchain::Swapchain(const Extent2D& requiredExtentInPixels)
{
	using namespace SwapchainDetails;

	const VkSurfaceCapabilitiesKHR surfaceCapabilities = GetSurfaceCapabilities(VulkanContext::device->physicalDevice);
	supportDetails = GetSwapchainSupportDetails(VulkanContext::device->physicalDevice);
	surfaceFormat = SelectSurfaceFormat(supportDetails.formats);	
	extent = SelectExtent(surfaceCapabilities, requiredExtentInPixels);

	swapchain = CreateSwapchain(supportDetails, surfaceCapabilities, surfaceFormat, extent);

	images = GetSwapchainImages(swapchain);
	imageViews = CreateImageViews(images, surfaceFormat.format);
}

void Swapchain::Cleanup()
{
	for (const auto imageView : imageViews)
	{
		vkDestroyImageView(VulkanContext::device->device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(VulkanContext::device->device, swapchain, nullptr);
}

Swapchain::~Swapchain()
{
	Cleanup();
}

void Swapchain::Recreate(const Extent2D& requiredExtentInPixels)
{
	using namespace SwapchainDetails;

	Cleanup();

	const VkSurfaceCapabilitiesKHR surfaceCapabilities = GetSurfaceCapabilities(VulkanContext::device->physicalDevice);
	extent = SelectExtent(surfaceCapabilities, requiredExtentInPixels);
	
	swapchain = CreateSwapchain(supportDetails, surfaceCapabilities, surfaceFormat, extent);

	images = GetSwapchainImages(swapchain);
	imageViews = CreateImageViews(images, surfaceFormat.format);
}