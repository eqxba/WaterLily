#pragma once

#include "Engine/Render/RenderStages/RenderStage.hpp"
#include "Engine/Render/Vulkan/Pipelines/Pipeline.hpp"
#include "Engine/Render/Vulkan/DescriptorSets/DescriptorSetLayout.hpp"

class PrimitiveCullStage : public RenderStage
{
public:
    PrimitiveCullStage(const VulkanContext& vulkanContext, RenderContext& renderContext);
    ~PrimitiveCullStage() override;

    void Prepare(const Scene& scene) override;

    void Execute(const Frame& frame) override;

    void TryReloadShaders() override;

private:
    Pipeline CreatePipeline() const;

    DescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    Pipeline pipeline;
};
