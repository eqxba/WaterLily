#pragma once

#include <volk.h>

class RenderPass
{
public:
	RenderPass();
	~RenderPass();

	VkRenderPass renderPass;
};