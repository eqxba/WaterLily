#include "Engine/Render/Vulkan/Device.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/Instance.hpp"
#include "Engine/Render/Vulkan/Surface.hpp"

namespace DeviceDetails
{
	static bool ExtensionsSupported(VkPhysicalDevice device, const std::vector<const char*>& extensions)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
		
		for (const auto& extension : extensions)
		{
			const auto isExtensionSupported = [&extension](const auto& extensionProperties)
			{
				return std::strcmp(extension, extensionProperties.extensionName) == 0;
			};

			const auto it = std::ranges::find_if(availableExtensions, isExtensionSupported);

			if (it == availableExtensions.end())
			{
				LogE << "Extension not supported: " << extension << std::endl;
				return false;
			}
		}

		return true;
	}
	
	static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices{};
		
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		for (uint32_t i = 0; i < queueFamilyCount; ++i)
		{
			if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, VulkanContext::surface->surface, &presentSupport);

			if (presentSupport)
			{
				indices.presentationFamily = i;
			}

			if (indices.IsComplete())
			{
				break;
			}
		}
		
		return indices;
	}

	static Queues GetQueues(VkPhysicalDevice physicalDevice, VkDevice device)
	{
		Queues queues{};
		QueueFamilyIndices indices = DeviceDetails::FindQueueFamilies(physicalDevice);
		
		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &queues.graphics);
		vkGetDeviceQueue(device, indices.presentationFamily.value(), 0, &queues.presentation);

		return queues;
	}
	
	static bool IsPhysicalDeviceSuitable(VkPhysicalDevice device)
	{
		const QueueFamilyIndices indices = FindQueueFamilies(device);
		return indices.IsComplete() && ExtensionsSupported(device, VulkanConfig::requiredDeviceExtensions);
	}

	// TODO: (low priority) device selection based on some kind of score (do i really need this?)
	static VkPhysicalDevice SelectPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(VulkanContext::instance->instance, &deviceCount, nullptr);
		Assert(deviceCount != 0);
		
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(VulkanContext::instance->instance, &deviceCount, devices.data());

		const auto it = std::ranges::find_if(devices, IsPhysicalDeviceSuitable);
		Assert(it != devices.end());

		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(*it, &deviceProperties);

		LogI << "Selected physical device: " << deviceProperties.deviceName << std::endl;

		return *it;
	}

	static VkDevice SelectLogicalDevice(VkPhysicalDevice physicalDevice)
	{
		QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);

		float queuePriority = 1.0f;
		
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentationFamily.value() };

		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(VulkanConfig::requiredDeviceExtensions.size());
		createInfo.ppEnabledExtensionNames = VulkanConfig::requiredDeviceExtensions.data();
		// In previous sdk versions it was required to duplicate provided to instance layers but anyways i'm gonna use
		// ray tracing so screw it
		createInfo.enabledLayerCount = 0;

		VkDevice device;
		const VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
		Assert(result == VK_SUCCESS);

		return device;
	}
}

Device::Device()
{
	physicalDevice = DeviceDetails::SelectPhysicalDevice();
	device = DeviceDetails::SelectLogicalDevice(physicalDevice);

	volkLoadDevice(device);

	queues = DeviceDetails::GetQueues(physicalDevice, device);
}

Device::~Device()
{
	vkDestroyDevice(device, nullptr);
}
