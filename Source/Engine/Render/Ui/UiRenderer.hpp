#pragma once

#include <volk.h>

#include "Utils/Constants.hpp"
#include "Utils/DataStructures.hpp"
#include "Engine/Render/Renderer.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/Buffer/Buffer.hpp"
#include "Engine/Render/Vulkan/Image/Texture.hpp"
#include "Engine/Render/Vulkan/Pipelines/Pipeline.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderModule.hpp"
#include "Engine/Render/Vulkan/DescriptorSets/DescriptorSetLayout.hpp"

class VulkanContext;
class EventSystem;
class Window;
class Widget;

namespace ES
{
    struct BeforeSwapchainRecreated;
    struct SwapchainRecreated;
    struct BeforeWindowRecreated;
    struct WindowRecreated;
    struct TryReloadShaders;
    struct BeforeInputModeUpdated;
}

class UiRenderer : public Renderer
{
public:
    struct PushConstants
    {
        glm::vec2 scale = Vector2::allOnes;
        glm::vec2 translate = Vector2::allMinusOnes;
    };

    UiRenderer(const Window& window, EventSystem& eventSystem, const VulkanContext& vulkanContext);
    ~UiRenderer() override;

    UiRenderer(const UiRenderer&) = delete;
    UiRenderer& operator=(const UiRenderer&) = delete;

    UiRenderer(UiRenderer&&) = delete;
    UiRenderer& operator=(UiRenderer&&) = delete;

    void Process(float deltaSeconds) override;

    void Render(const Frame& frame) override;

private:
    void CreateGraphicsPipeline(std::vector<ShaderModule>&& shaderModules);
    
    void UpdateBuffers(uint32_t frameIndex);
    
    void UpdateImGuiInputState() const;

    void OnBeforeSwapchainRecreated(const ES::BeforeSwapchainRecreated& event);
    void OnSwapchainRecreated(const ES::SwapchainRecreated& event);
    void OnBeforeWindowRecreated(const ES::BeforeWindowRecreated& event);
    void OnWindowRecreated(const ES::WindowRecreated& event);
    void OnTryReloadShaders(const ES::TryReloadShaders& event);
    void OnBeforeInputModeUpdated(const ES::BeforeInputModeUpdated& event);

    const VulkanContext* vulkanContext = nullptr;

    EventSystem* eventSystem = nullptr;

    const Window* window = nullptr;

    RenderPass renderPass;
    Pipeline graphicsPipeline;

    std::vector<VkFramebuffer> framebuffers;

    std::vector<Buffer> vertexBuffers;
    std::vector<Buffer> indexBuffers;

    Texture fontTexture;

    VkDescriptorSet descriptor = VK_NULL_HANDLE;
    DescriptorSetLayout layout;

    PushConstants pushConstants;

    InputMode inputMode = InputMode::eEngine;
    
    std::vector<std::unique_ptr<Widget>> widgets;
};
