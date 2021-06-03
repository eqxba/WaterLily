#include "Engine/Render/Vulkan/Shaders/ShaderModule.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace ShaderModuleDetails
{
	static VkShaderStageFlagBits GetVkShaderStageFlagBits(const ShaderType shaderType)
	{
		switch (shaderType)
		{
			case ShaderType::eVertex:
				return VK_SHADER_STAGE_VERTEX_BIT;
			case ShaderType::eFragment:
				return VK_SHADER_STAGE_FRAGMENT_BIT;
			// TODO: Ask BigBoss about this stuff
			default:
				Assert(false);
				return VK_SHADER_STAGE_ALL;
		}
	}
}

ShaderModule::ShaderModule(const VkShaderModule aShaderModule, const ShaderType aShaderType)
	: shaderModule(aShaderModule)
	, shaderType(aShaderType)
{}

ShaderModule::~ShaderModule()
{
	vkDestroyShaderModule(VulkanContext::device->device, shaderModule, nullptr);
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