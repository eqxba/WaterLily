#pragma once

class Instance;

class VulkanContext
{
public:
	static void Create();
	static void Destroy();

	static std::unique_ptr<Instance> instance;
};