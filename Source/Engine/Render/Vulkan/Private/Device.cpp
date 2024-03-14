#include "Engine/Render/Vulkan/Device.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"

namespace DeviceDetails
{	
	static bool ExtensionsSupported(VkPhysicalDevice device, const std::vector<const char*>& extensions)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensionsProperties(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensionsProperties.data());

		const auto isSupported = [&](const char* extension) {
			const bool isSupported = std::ranges::any_of(availableExtensionsProperties, [=](const auto& properties) {
				return std::strcmp(extension, properties.extensionName) == 0;
			});

			if (!isSupported)
			{
				LogE << "Extension not supported: " << extension << '\n';
			}

			return isSupported;
		};

		return std::ranges::all_of(extensions, isSupported);	
	}

	static std::optional<uint32_t> FindGraphicsQueueFamilyIndex(const std::vector<VkQueueFamilyProperties>& queueFamilies)
	{
		const auto isGraphicsFamily = [](const VkQueueFamilyProperties& properties) {
			return properties.queueFlags & VK_QUEUE_GRAPHICS_BIT;
		};

		const auto it = std::ranges::find_if(queueFamilies, isGraphicsFamily);

		return it == queueFamilies.end() ? std::nullopt
			: std::optional(static_cast<uint32_t>(std::distance(queueFamilies.begin(), it)));
	}

	static std::optional<uint32_t> FindPresentQueueFamilyIndex(const std::vector<VkQueueFamilyProperties>& queueFamilies,
		VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		for (uint32_t i = 0; i < queueFamilies.size(); ++i)
		{
			VkBool32 presentSupport = false;

			const VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			Assert(result == VK_SUCCESS);

			if (presentSupport)
			{
				return i;
			}
		}

		return std::nullopt;
	}
	
	static QueueFamilyIndices GetQueueFamilyIndices(const VulkanContext& vulkanContext, VkPhysicalDevice device)
	{						
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		std::optional<uint32_t> graphicsFamily = FindGraphicsQueueFamilyIndex(queueFamilies);
		Assert(graphicsFamily.has_value());

		std::optional<uint32_t> presentFamily = FindPresentQueueFamilyIndex(queueFamilies, device, vulkanContext.GetSurface().GetVkSurfaceKHR());
		Assert(presentFamily.has_value());		
		
		return { graphicsFamily.value(), presentFamily.value()};
	}

	static Queues GetQueues(const VulkanContext& vulkanContext, VkPhysicalDevice physicalDevice, VkDevice device)
	{
		Queues queues{};
		queues.familyIndices = DeviceDetails::GetQueueFamilyIndices(vulkanContext, physicalDevice);
		
		vkGetDeviceQueue(device, queues.familyIndices.graphicsFamily, 0, &queues.graphics);
		vkGetDeviceQueue(device, queues.familyIndices.presentFamily, 0, &queues.present);

		return queues;
	}
	
	static bool IsPhysicalDeviceSuitable(VkPhysicalDevice device)
	{
		return ExtensionsSupported(device, VulkanConfig::requiredDeviceExtensions);
	}

	// TODO: (low priority) device selection based on some kind of score (do i really need this?)
	static VkPhysicalDevice SelectPhysicalDevice(const VulkanContext& vulkanContext)
	{
		const VkInstance vkInstance = vulkanContext.GetInstance().GetVkInstance();

		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);
		Assert(deviceCount != 0);
		
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data());

		const auto it = std::ranges::find_if(devices, IsPhysicalDeviceSuitable);
		Assert(it != devices.end());

		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(*it, &deviceProperties);

		LogI << "Selected physical device: " << deviceProperties.deviceName << std::endl;

		return *it;
	}

	static VkDevice SelectLogicalDevice(const VulkanContext& vulkanContext, VkPhysicalDevice physicalDevice)
	{
		QueueFamilyIndices indices = GetQueueFamilyIndices(vulkanContext, physicalDevice);

		float queuePriority = 1.0f;
		
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilyIndices = { indices.graphicsFamily, indices.presentFamily };

		const auto addQueueCreateInfo = [&](uint32_t queueFamilyIndex) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.emplace_back(queueCreateInfo);
		};

		std::ranges::for_each(uniqueQueueFamilyIndices, addQueueCreateInfo);

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

	static VkCommandPool CreateCommandPool(VkDevice device, VkCommandPoolCreateFlags flags, uint32_t queueFamilyIndex)
	{		
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndex;
		poolInfo.flags = flags;

		VkCommandPool commandPool;
		const VkResult result = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
		Assert(result == VK_SUCCESS);

		return commandPool;
	}
}

Device::Device(const VulkanContext& aVulkanContext)
	: vulkanContext{aVulkanContext}
{
	using namespace DeviceDetails;

	physicalDevice = SelectPhysicalDevice(vulkanContext);
	device = SelectLogicalDevice(vulkanContext, physicalDevice);

	volkLoadDevice(device);

	queues = DeviceDetails::GetQueues(vulkanContext, physicalDevice, device);

	commandPools.emplace(CommandBufferType::eLongLived, 
		CreateCommandPool(device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queues.familyIndices.graphicsFamily));
	commandPools.emplace(CommandBufferType::eOneTime, 
		CreateCommandPool(device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | 
		VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, queues.familyIndices.graphicsFamily));

	oneTimeCommandBufferSync.fence = VulkanHelpers::CreateFence(device, {});
}

Device::~Device()
{
	VulkanHelpers::DestroyCommandBufferSync(device, oneTimeCommandBufferSync);

	std::ranges::for_each(commandPools, [&](auto& entry) {
		vkDestroyCommandPool(device, entry.second, nullptr);
	});

	vkDestroyDevice(device, nullptr);
}

void Device::WaitIdle() const
{
	vkDeviceWaitIdle(device);
}

VkCommandPool Device::GetCommandPool(CommandBufferType type) const
{
    return commandPools[type];
}

void Device::ExecuteOneTimeCommandBuffer(DeviceCommands commands) const
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPools[CommandBufferType::eOneTime];
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer oneTimeBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &oneTimeBuffer);
	
    VulkanHelpers::SubmitCommandBuffer(oneTimeBuffer, queues.graphics, commands, oneTimeCommandBufferSync);

    vkWaitForFences(device, 1, &oneTimeCommandBufferSync.fence, VK_TRUE, UINT64_MAX);

	// TODO: (low priority) find out if we have to allocate and free each time or it's ok to have 1 buffer persistent
	// Also see VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT flag
	vkFreeCommandBuffers(device, commandPools[CommandBufferType::eOneTime], 1, &oneTimeBuffer);

	const VkResult result = vkResetFences(device, 1, &oneTimeCommandBufferSync.fence);
    Assert(result == VK_SUCCESS);
}

