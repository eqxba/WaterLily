#pragma once

#include "Engine/Systems/System.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"

#include <volk.h>

class RenderSystem : public System
{
public:
	RenderSystem();
	~RenderSystem() override;

	void Process(float deltaSeconds) override;

	void Render();

private:
	RenderPass renderPass;
	GraphicsPipeline graphicsPipeline;
	std::vector<VkFramebuffer> swapchainFramebuffers;
};