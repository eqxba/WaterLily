#pragma once

#include <volk.h>

#include "Engine/Render/Renderer.hpp"
#include "Engine/Render/Vulkan/Pipelines/Pipeline.hpp"
#include "Engine/Render/Vulkan/Image/RenderTarget.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderModule.hpp"
#include "Engine/Render/Vulkan/DescriptorSets/DescriptorSetLayout.hpp"

class VulkanContext;
class EventSystem;

class ComputeRenderer : public Renderer
{
public:
    ComputeRenderer(EventSystem& eventSystem, const VulkanContext& vulkanContext);
    ~ComputeRenderer() override;

    ComputeRenderer(const ComputeRenderer&) = delete;
    ComputeRenderer& operator=(const ComputeRenderer&) = delete;

    ComputeRenderer(ComputeRenderer&&) = delete;
    ComputeRenderer& operator=(ComputeRenderer&&) = delete;

    void Process(const Frame& frame ,float deltaSeconds) override;

    void Render(const Frame& frame) override;

private:
    void CreatePipeline(ShaderModule&& shaderModule);
    void CreateDescriptors();

    void OnBeforeSwapchainRecreated();
    void OnSwapchainRecreated();
    void OnTryReloadShaders();

    const VulkanContext* vulkanContext = nullptr;

    EventSystem* eventSystem = nullptr;

    Pipeline computePipeline;

    RenderTarget renderTarget;

    std::vector<VkFramebuffer> framebuffers;

    std::vector<VkDescriptorSet> descriptors;
};
