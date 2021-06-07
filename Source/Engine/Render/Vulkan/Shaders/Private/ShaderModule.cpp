#include "Engine/Render/Vulkan/Shaders/ShaderModule.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace ShaderModuleDetails
{
	static VkShaderStageFlagBits GetVkShaderStageFlagBits(const ShaderType shaderType)
	{
		VkShaderStageFlagBits result{};
		
		switch (shaderType)
		{
			case ShaderType::eVertex:
				result = VK_SHADER_STAGE_VERTEX_BIT;
				break;
			case ShaderType::eFragment:
				result = VK_SHADER_STAGE_FRAGMENT_BIT;
				break;
		}

		return result;
	}
}

ShaderModule::ShaderModule(const VkShaderModule aShaderModule, const ShaderType aShaderType)
	: shaderModule(aShaderModule)
	, shaderType(aShaderType)
{}

ShaderModule::ShaderModule(ShaderModule&& other)
	: shaderModule(other.shaderModule)
	, shaderType(other.shaderType)
{
	other.shaderModule = VK_NULL_HANDLE;
}

ShaderModule::~ShaderModule()
{
	if (shaderModule != VK_NULL_HANDLE)
	{ 
		vkDestroyShaderModule(VulkanContext::device->device, shaderModule, nullptr);
	}	
}

VkPipelineShaderStageCreateInfo ShaderModule::GetVkPipelineShaderStageCreateInfo() const
{
	VkPipelineShaderStageCreateInfo shaderStageCreateInfo{};
	shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo.stage = ShaderModuleDetails::GetVkShaderStageFlagBits(shaderType);
	shaderStageCreateInfo.module = shaderModule;
	shaderStageCreateInfo.pName = "main"; // Let's use "main" entry point by default

	return shaderStageCreateInfo;
}