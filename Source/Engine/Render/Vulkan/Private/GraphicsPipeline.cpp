#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace GraphicsPipelineDetails
{
	// TODO: Add FileSystem and remove absolute paths
	constexpr const char* vertexShaderPath = "C:/Users/eqxba/Projects/WaterLily/Source/Shaders/shader.spv";
	constexpr const char* fragmentShaderPath = "C:/Users/eqxba/Projects/WaterLily/Source/Shaders/shader.spv";
}

GraphicsPipeline::GraphicsPipeline()
{
	using namespace GraphicsPipelineDetails;
	
	const ShaderModule vertexShader =
		VulkanContext::shaderManager->CreateShaderModule(vertexShaderPath, ShaderType::eVertex);
	const ShaderModule fragmentShader =
		VulkanContext::shaderManager->CreateShaderModule(fragmentShaderPath, ShaderType::eFragment);

	VkPipelineShaderStageCreateInfo shaderStages[] =
	{
		vertexShader.GetVkPipelineShaderStageCreateInfo(),
		fragmentShader.GetVkPipelineShaderStageCreateInfo()
	};
}

GraphicsPipeline::~GraphicsPipeline()
{
	
}