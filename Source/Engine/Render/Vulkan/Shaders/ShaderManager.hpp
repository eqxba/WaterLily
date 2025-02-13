#pragma once

#include "Engine/Render/Vulkan/Shaders/ShaderModule.hpp"
#include "Engine/FileSystem/FileSystem.hpp"

class VulkanContext;

class ShaderManager
{
public:
	ShaderManager(const VulkanContext& vulkanContext);

    ShaderModule CreateShaderModule(const FilePath& path, const ShaderType shaderType) const;

private:
	const VulkanContext& vulkanContext;
};
