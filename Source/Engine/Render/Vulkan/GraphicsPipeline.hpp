#pragma once

#include <volk.h>

class VulkanContext;
class RenderPass;

class GraphicsPipeline
{
public:
	// TODO: Pass shaders (an probably other things) as parameters
	GraphicsPipeline(const RenderPass& renderPass, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
		const VulkanContext& aVulkanContext);
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

private:
	const VulkanContext& vulkanContext;

	VkPipelineLayout pipelineLayout;

	VkPipeline pipeline;
};