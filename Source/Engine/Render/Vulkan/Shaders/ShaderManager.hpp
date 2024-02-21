#pragma once

#include "Engine/Render/Vulkan/Shaders/ShaderModule.hpp"

class VulkanContext;

class ShaderManager
{
public:
	ShaderManager(const VulkanContext& vulkanContext);

	ShaderModule CreateShaderModule(const std::string& filePath, const ShaderType shaderType) const;

private:
	const VulkanContext& vulkanContext;
};