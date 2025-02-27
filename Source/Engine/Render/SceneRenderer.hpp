#pragma once

#include <volk.h>

#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/Pipeline.hpp"
#include "Engine/Render/Resources/Shaders/ShaderModule.hpp"
#include "Engine/Render/Resources/DescriptorSets/DescriptorSetLayout.hpp"
#include "Engine/Render/Frame.hpp"
#include "Shaders/Common.h"

class VulkanContext;
class EventSystem;
class Image;
class ImageView;
class Buffer;
class Scene;

namespace ES
{
    struct BeforeSwapchainRecreated;
    struct SwapchainRecreated;
    struct TryReloadShaders;
    struct SceneOpened;
    struct SceneClosed;
}

class SceneRenderer
{
public:
    SceneRenderer(EventSystem& eventSystem, const VulkanContext& vulkanContext);
    ~SceneRenderer();

    SceneRenderer(const SceneRenderer&) = delete;
    SceneRenderer& operator=(const SceneRenderer&) = delete;

    SceneRenderer(SceneRenderer&&) = delete;
    SceneRenderer& operator=(SceneRenderer&&) = delete;

    void Process(float deltaSeconds);

    void Render(const Frame& frame);

private:
    void CreateGraphicsPipeline(std::vector<ShaderModule>&& shaderModules);

    void CreateAttachmentsAndFramebuffers();
    void DestroyAttachmentsAndFramebuffers();

    void OnBeforeSwapchainRecreated(const ES::BeforeSwapchainRecreated& event);
    void OnSwapchainRecreated(const ES::SwapchainRecreated& event);
    void OnTryReloadShaders(const ES::TryReloadShaders& event);
    void OnSceneOpen(const ES::SceneOpened& event);
    void OnSceneClose(const ES::SceneClosed& event);

    const VulkanContext* vulkanContext = nullptr;

    EventSystem* eventSystem = nullptr;

    RenderPass renderPass;
    Pipeline graphicsPipeline;

    std::unique_ptr<Image> colorAttachment;
    std::unique_ptr<ImageView> colorAttachmentView;

    std::unique_ptr<Image> depthAttachment;
    std::unique_ptr<ImageView> depthAttachmentView;

    std::vector<VkFramebuffer> framebuffers;

    std::unique_ptr<Buffer> indirectBuffer;
    uint32_t indirectDrawCount = 0;

    std::vector<Buffer> uniformBuffers;

    std::vector<VkDescriptorSet> uniformDescriptors;
    DescriptorSetLayout layout;

    gpu::UniformBufferObject ubo = { .view = glm::mat4(), .projection = glm::mat4() };

    Scene* scene = nullptr;
};
