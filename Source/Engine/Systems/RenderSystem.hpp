#pragma once

#include "Engine/Systems/System.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"

#include <volk.h>

namespace ES
{
	struct WindowResized;
}

class VulkanContext;
class Scene;
class EventSystem;
class CommandBufferSync;

class RenderSystem : public System
{
public:
	RenderSystem(Scene& aScene, EventSystem& aEventSystem, const VulkanContext& vulkanContext);
	~RenderSystem() override;

	RenderSystem(const RenderSystem&) = delete;
	RenderSystem& operator=(const RenderSystem&) = delete;

	RenderSystem(RenderSystem&&) = delete;
	RenderSystem& operator=(RenderSystem&&) = delete;

	void Process(float deltaSeconds) override;

	void Render();

private:
	void OnResize(const ES::WindowResized& event);

	const VulkanContext& vulkanContext;

	EventSystem& eventSystem;
	
	// Per "stage"
	std::unique_ptr<RenderPass> renderPass;
	std::unique_ptr <GraphicsPipeline> graphicsPipeline;
	std::vector<VkFramebuffer> framebuffers;
	
	// Do not have to be recreated, persistent for FrameLoop
	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<CommandBufferSync> syncs;
	
	size_t currentFrame = 0;

	Scene& scene;
};