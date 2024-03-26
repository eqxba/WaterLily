#pragma once

#include <volk.h>

class VulkanContext;
class RenderPass;

class GraphicsPipeline
{
public:
	// TODO: Pass shaders (an probably other things) as parameters
	GraphicsPipeline(const RenderPass& renderPass, const VulkanContext& aVulkanContext);
	~GraphicsPipeline();

	GraphicsPipeline(const GraphicsPipeline&) = delete;
	GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

	GraphicsPipeline(GraphicsPipeline&&) = delete;
	GraphicsPipeline& operator=(GraphicsPipeline&&) = delete;

	VkPipeline GetVkPipeline() const
	{
		return pipeline;
	}

	VkPipelineLayout GetPipelineLayout() const
	{
		return pipelineLayout;
	}

	const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const 
	{ 
		return descriptorSetLayouts; 
	}

private:
	const VulkanContext& vulkanContext;

	VkPipelineLayout pipelineLayout;

	VkPipeline pipeline;

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
};