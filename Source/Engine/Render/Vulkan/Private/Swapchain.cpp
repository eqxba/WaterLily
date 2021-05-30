#include "Engine/Render/Vulkan/Swapchain.hpp"

#include "Engine/Window.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Surface.hpp"

namespace SwapchainDetails
{
	static SwapchainSupportDetails GetSwapchainSupportDetails(VkPhysicalDevice device)
	{
		SwapchainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, VulkanContext::surface->surface, &details.capabilities);

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

	static VkExtent2D SelectExtent(const VkSurfaceCapabilitiesKHR& capabilities, const Window& window)
	{
		// currentExtent is the current width and height of the surface, or the special value (0xFFFFFFFF, 0xFFFFFFFF)
		// indicating that the surface size will be determined by the extent of a swapchain targeting the surface.
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}
		else
		{
			const Extent2D windowExtent = window.GetExtentInPixels();

			const VkExtent2D actualExtent = {
				std::clamp(static_cast<uint32_t>(windowExtent.width),
					capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
				std::clamp(static_cast<uint32_t>(windowExtent.height),
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
}

Swapchain::Swapchain(const Window& window)
{
	using namespace SwapchainDetails;
	
	SwapchainSupportDetails swapChainSupport = GetSwapchainSupportDetails(VulkanContext::device->physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = SelectSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = SelectPresentMode(swapChainSupport.presentModes);
	extent = SelectExtent(swapChainSupport.capabilities, window);
	uint32_t imageCount = SelectImageCount(swapChainSupport.capabilities);

	format = surfaceFormat.format;

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = VulkanContext::surface->surface;
	createInfo.minImageCount = imageCount;
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
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;
	
	const VkResult result = vkCreateSwapchainKHR(VulkanContext::device->device, &createInfo, nullptr, &swapchain);
	Assert(result == VK_SUCCESS);

	vkGetSwapchainImagesKHR(VulkanContext::device->device, swapchain, &imageCount, nullptr);
	images.resize(imageCount);
	vkGetSwapchainImagesKHR(VulkanContext::device->device, swapchain, &imageCount, images.data());
}

Swapchain::~Swapchain()
{
	vkDestroySwapchainKHR(VulkanContext::device->device, swapchain, nullptr);
}