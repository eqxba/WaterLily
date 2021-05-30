#pragma once

class Window;
class Instance;
class Surface;
class Device;
class Swapchain;

class VulkanContext
{
public:
	static void Create(const Window& window);
	static void Destroy();

	static std::unique_ptr<Instance> instance;
	static std::unique_ptr<Surface> surface;
	static std::unique_ptr<Device> device;
	static std::unique_ptr<Swapchain> swapchain;
};