#pragma once

#include "Engine/Systems/System.hpp"
#include "Utils/Constants.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"
#include "Engine/Render/Vulkan/Resources/CommandBufferSync.hpp"
#include "Shaders/Common.h"

#include <volk.h>

namespace ES
{
	struct WindowResized;
	struct BeforeWindowRecreated;
	struct WindowRecreated;
	struct SceneOpened;
}

class VulkanContext;
class Scene;
class EventSystem;
class CommandBufferSync;
class Buffer;
class Image;
class ImageView;

class RenderSystem : public System
{
public:
	RenderSystem(EventSystem& aEventSystem, const VulkanContext& vulkanContext);
	~RenderSystem() override;

	RenderSystem(const RenderSystem&) = delete;
	RenderSystem& operator=(const RenderSystem&) = delete;

	RenderSystem(RenderSystem&&) = delete;
	RenderSystem& operator=(RenderSystem&&) = delete;

	void Process(float deltaSeconds) override;

	void Render();

private:
	void OnResize(const ES::WindowResized& event);
	void OnBeforeWindowRecreated(const ES::BeforeWindowRecreated& event);
	void OnWindowRecreated(const ES::WindowRecreated& event);
	void OnSceneOpen(const ES::SceneOpened& event);

	void CreateAttachmentsAndFramebuffers();
	void DestroyAttachmentsAndFramebuffers();

	const VulkanContext& vulkanContext;

	EventSystem& eventSystem;

	std::unique_ptr<RenderPass> renderPass;
	std::unique_ptr<GraphicsPipeline> graphicsPipeline;

	std::unique_ptr<Image> colorAttachment;
	std::unique_ptr<ImageView> colorAttachmentView;

	std::unique_ptr<Image> depthAttachment;
	std::unique_ptr<ImageView> depthAttachmentView;

	std::vector<VkFramebuffer> framebuffers;
	
	// Do not have to be recreated, persistent for FrameLoop
	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<CommandBufferSync> syncs;

	// TODO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	gpu::UniformBufferObject ubo = { .view = glm::mat4(), .projection = glm::mat4() };

	std::vector<Buffer> uniformBuffers;

	std::unique_ptr<Buffer> indirectBuffer;
	uint32_t indirectDrawCount = 0;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
	
	size_t currentFrame = 0;

	Scene* scene = nullptr;
};