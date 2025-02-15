#pragma once

#include "Engine/Systems/System.hpp"
#include "Utils/Constants.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"
#include "Engine/Render/Vulkan/Frame.hpp"
#include "Shaders/Common.h"

#include <volk.h>

namespace ES
{
	struct WindowResized;
	struct BeforeWindowRecreated;
	struct WindowRecreated;
	struct SceneOpened;
    struct SceneClosed;
}

class VulkanContext;
class Device;
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
	uint32_t AcquireNextImage(const Frame& frame) const;
	void RenderScene(const Frame& frame, VkFramebuffer framebuffer) const;
	void Present(const Frame& frame, uint32_t imageIndex) const;

	void CreateAttachmentsAndFramebuffers();
	void DestroyAttachmentsAndFramebuffers();

	void OnResize(const ES::WindowResized& event);
	void OnBeforeWindowRecreated(const ES::BeforeWindowRecreated& event);
	void OnWindowRecreated(const ES::WindowRecreated& event);
	void OnSceneOpen(const ES::SceneOpened& event);
    void OnSceneClose(const ES::SceneClosed& event);

	const VulkanContext& vulkanContext;
	const Device& device;

	EventSystem& eventSystem;

	std::unique_ptr<RenderPass> renderPass;
	std::unique_ptr<GraphicsPipeline> graphicsPipeline;

	std::unique_ptr<Image> colorAttachment;
	std::unique_ptr<ImageView> colorAttachmentView;

	std::unique_ptr<Image> depthAttachment;
	std::unique_ptr<ImageView> depthAttachmentView;

	std::vector<VkFramebuffer> framebuffers;
    
    std::vector<Frame> frames;
    size_t currentFrame = 0;
    
	gpu::UniformBufferObject ubo = { .view = glm::mat4(), .projection = glm::mat4() };

	std::unique_ptr<Buffer> indirectBuffer;
	uint32_t indirectDrawCount = 0;

	Scene* scene = nullptr;
};
