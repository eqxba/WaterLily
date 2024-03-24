#pragma once

#include "Engine/Render/Vulkan/Shaders/ShaderManager.hpp"
#include "Engine/Render/Vulkan/Instance.hpp"
#include "Engine/Render/Vulkan/Surface.hpp"
#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Swapchain.hpp"
#include "Engine/Render/Vulkan/Resources/MemoryManager.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderManager.hpp"

class Window;

class VulkanContext
{
public:
	VulkanContext(const Window& window);
	~VulkanContext() = default;

	VulkanContext(const VulkanContext&) = delete;
	VulkanContext& operator=(const VulkanContext&) = delete;

	VulkanContext(VulkanContext&&) = delete;
	VulkanContext& operator=(VulkanContext&&) = delete;

	const Instance& GetInstance() const
	{
		return *instance;
	}

	const Surface& GetSurface() const
	{
		return *surface;
	}

	const Device& GetDevice() const
	{
		return *device;
	}

	Swapchain& GetSwapchain() const
	{
		return *swapchain;
	}

	MemoryManager& GetMemoryManager() const
	{
		return *memoryManager;
	}

	const ShaderManager& GetShaderManager() const
	{
		return *shaderManager;
	}

private:
	std::unique_ptr<Instance> instance;
	std::unique_ptr<Surface> surface;
	std::unique_ptr<Device> device;
	std::unique_ptr<Swapchain> swapchain;

	std::unique_ptr<MemoryManager> memoryManager;
	std::unique_ptr<ShaderManager> shaderManager;
};