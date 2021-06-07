#pragma once

#include <volk.h>

enum class ShaderType
{
	eVertex,
	eFragment
};

class ShaderModule
{
public:
	ShaderModule(const VkShaderModule shaderModule, const ShaderType shaderType);
	ShaderModule(ShaderModule&& other);
	~ShaderModule();

	VkPipelineShaderStageCreateInfo GetVkPipelineShaderStageCreateInfo() const;
	
	VkShaderModule shaderModule;
	ShaderType shaderType;
};