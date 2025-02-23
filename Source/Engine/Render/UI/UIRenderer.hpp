#pragma once

#include <volk.h>

DISABLE_WARNINGS_BEGIN
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
DISABLE_WARNINGS_END

#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Resources/Buffer.hpp"
#include "Engine/Render/Resources/Image.hpp"
#include "Engine/Render/Resources/ImageView.hpp"
#include "Engine/Render/Resources/DescriptorSets/DescriptorSetLayout.hpp"
#include "Utils/Constants.hpp"

class VulkanContext;
class EventSystem;
class Window;

namespace ES
{
    struct BeforeSwapchainRecreated;
    struct SwapchainRecreated;
	struct BeforeWindowRecreated;
	struct WindowRecreated;
	struct BeforeCursorModeUpdated;
}

class UIRenderer
{
public:
	struct PushConstants
	{
		glm::vec2 scale = Vector2::allOnes;
		glm::vec2 translate = Vector2::allMinusOnes;
	};

	UIRenderer(const Window& window, EventSystem& eventSystem, const VulkanContext& vulkanContext);
	~UIRenderer();

	UIRenderer(const UIRenderer&) = delete;
	UIRenderer& operator=(const UIRenderer&) = delete;

	UIRenderer(UIRenderer&&) = delete;
	UIRenderer& operator=(UIRenderer&&) = delete;

	void Process(float deltaSeconds);

	void Render(VkCommandBuffer commandBuffer, uint32_t swapchainImageIndex);

	void TryReloadShaders();

private:
	void CreateGraphicsPipeline(std::vector<ShaderModule>&& shaderModules);

	void BuildUi();
	void UpdateBuffers();
    
    void UpdateDisplaySize() const;

    void OnBeforeSwapchainRecreated(const ES::BeforeSwapchainRecreated& event);
    void OnSwapchainRecreated(const ES::SwapchainRecreated& event);
	void OnBeforeWindowRecreated(const ES::BeforeWindowRecreated& event);
	void OnWindowRecreated(const ES::WindowRecreated& event);
	void OnBeforeCursorModeUpdated(const ES::BeforeCursorModeUpdated& event);

	const VulkanContext* vulkanContext = nullptr;

	EventSystem* eventSystem = nullptr;

	const Window* window = nullptr;

	RenderPass renderPass;
	GraphicsPipeline graphicsPipeline;

	std::vector<VkFramebuffer> framebuffers;

	Buffer vertexBuffer;
	Buffer indexBuffer;

	Image fontImage;
	ImageView fontImageView;
	VkSampler fontImageSampler = VK_NULL_HANDLE;

	VkDescriptorSet descriptor = VK_NULL_HANDLE;
	DescriptorSetLayout layout;

	PushConstants pushConstants;

	CursorMode cursorMode = CursorMode::eDisabled;
};
