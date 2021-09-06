#pragma once

#include "Engine/Systems/System.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"

#include <volk.h>

class Scene;

class RenderSystem : public System
{
public:
	RenderSystem(Scene* aScene);
	~RenderSystem() override;

	void Process(float deltaSeconds) override;

	void Render();

private:
	void OnResize();
	
	// Per "stage"
	std::unique_ptr<RenderPass> renderPass;
	std::unique_ptr <GraphicsPipeline> graphicsPipeline;
	std::vector<VkFramebuffer> framebuffers;
	
	// Do not have to be recreated, persistent for FrameLoop
	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	
	size_t currentFrame = 0;

	Scene* scene;
};