#pragma once

#include <volk.h>

class RenderPass;

class GraphicsPipeline
{
public:
	// TODO: Pass shaders (an probably other things) as parameters
	GraphicsPipeline(const RenderPass& renderPass);
	~GraphicsPipeline();

	VkPipeline pipeline;

private:
	VkPipelineLayout pipelineLayout;
};