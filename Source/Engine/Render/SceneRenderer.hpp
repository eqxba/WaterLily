#pragma once

#include <volk.h>

#include "Shaders/Common.h"
#include "Engine/Render/Renderer.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/Buffer/Buffer.hpp"
#include "Engine/Render/Vulkan/Image/RenderTarget.hpp"
#include "Engine/Render/Vulkan/Pipelines/Pipeline.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderModule.hpp"
#include "Engine/Render/Vulkan/DescriptorSets/DescriptorSetLayout.hpp"

class VulkanContext;
class EventSystem;
class Scene;

namespace ES
{
    struct BeforeSwapchainRecreated;
    struct SwapchainRecreated;
    struct TryReloadShaders;
    struct SceneOpened;
    struct SceneClosed;
}

class SceneRenderer : public Renderer
{
public:
    SceneRenderer(EventSystem& eventSystem, const VulkanContext& vulkanContext);
    ~SceneRenderer() override;

    SceneRenderer(const SceneRenderer&) = delete;
    SceneRenderer& operator=(const SceneRenderer&) = delete;

    SceneRenderer(SceneRenderer&&) = delete;
    SceneRenderer& operator=(SceneRenderer&&) = delete;

    void Process(float deltaSeconds) override;

    void Render(const Frame& frame) override;

private:
    void CreateGraphicsPipeline(std::vector<ShaderModule>&& shaderModules);

    void CreateRenderTargetsAndFramebuffers();
    void DestroyRenderTargetsAndFramebuffers();

    void OnBeforeSwapchainRecreated(const ES::BeforeSwapchainRecreated& event);
    void OnSwapchainRecreated(const ES::SwapchainRecreated& event);
    void OnTryReloadShaders(const ES::TryReloadShaders& event);
    void OnSceneOpen(const ES::SceneOpened& event);
    void OnSceneClose(const ES::SceneClosed& event);

    const VulkanContext* vulkanContext = nullptr;

    EventSystem* eventSystem = nullptr;

    RenderPass renderPass;
    Pipeline graphicsPipeline;

    RenderTarget colorTarget;
    RenderTarget depthTarget;

    std::vector<VkFramebuffer> framebuffers;

    Buffer indirectBuffer;
    uint32_t indirectDrawCount = 0;

    std::vector<Buffer> uniformBuffers;

    std::vector<VkDescriptorSet> uniformDescriptors;
    DescriptorSetLayout layout;

    gpu::UniformBufferObject ubo = { .view = glm::mat4(), .projection = glm::mat4() };

    Scene* scene = nullptr;
};
