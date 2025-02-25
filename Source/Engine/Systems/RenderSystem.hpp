#pragma once

#include <volk.h>

#include "Engine/Systems/System.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/Frame.hpp"

namespace ES
{
    struct KeyInput;
}

class VulkanContext;
class Window;
class EventSystem;
class SceneRenderer;
class UiRenderer;

class RenderSystem : public System
{
public:
    RenderSystem(const Window& window, EventSystem& aEventSystem, const VulkanContext& vulkanContext);
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

    void OnKeyInput(const ES::KeyInput& event);

    const VulkanContext& vulkanContext;

    EventSystem& eventSystem;

    std::unique_ptr<SceneRenderer> sceneRenderer;
    std::unique_ptr<UiRenderer> uiRenderer;
    
    std::vector<Frame> frames;
    uint32_t currentFrame = 0;
};
