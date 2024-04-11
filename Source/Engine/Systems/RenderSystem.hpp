#pragma once

#include "Engine/Systems/System.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"
#include "Engine/Render/Vulkan/Resources/CommandBufferSync.hpp"
#include "Engine/Render/Vulkan/Resources/Image.hpp"
#include "Engine/Render/Vulkan/Resources/ImageView.hpp"
#include "Shaders/Common.h"

#include <volk.h>

namespace ES
{
	struct WindowResized;
}

class VulkanContext;
class Scene;
class EventSystem;
class CommandBufferSync;
class Buffer;

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

	std::unique_ptr<RenderPass> renderPass;
	std::unique_ptr<GraphicsPipeline> graphicsPipeline;


	Image colorAttachment;
	ImageView colorAttachmentView;

	Image depthAttachment;
	ImageView depthAttachmentView;

	std::vector<VkFramebuffer> framebuffers;
	
	// Do not have to be recreated, persistent for FrameLoop
	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<CommandBufferSync> syncs;

	gpu::UniformBufferObject ubo = { .model = glm::mat4(), .view = glm::mat4(), .projection = glm::mat4() };

	std::vector<Buffer> uniformBuffers;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
	
	size_t currentFrame = 0;

	Scene& scene;
};