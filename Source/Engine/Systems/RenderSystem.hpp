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

	// TODO: Temp
	void OnResize();
private:
	//void OnResize();
	
	std::unique_ptr<RenderPass> renderPass;
	std::unique_ptr <GraphicsPipeline> graphicsPipeline;
	std::vector<VkFramebuffer> framebuffers;
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;

	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	
	size_t currentFrame = 0;
};