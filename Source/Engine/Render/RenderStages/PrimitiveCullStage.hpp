#pragma once

#include "Engine/Render/RenderStages/RenderStage.hpp"
#include "Engine/Render/Vulkan/Pipelines/Pipeline.hpp"

class PrimitiveCullStage : public RenderStage
{
public:
    PrimitiveCullStage(const VulkanContext& vulkanContext, RenderContext& renderContext);
    ~PrimitiveCullStage() override;

    void Prepare(const Scene& scene) override;

    void Execute(const Frame& frame) override;

    void RecreatePipelinesAndDescriptors() override;
    
    void OnSceneClose() override;

private:
    Pipeline CreatePipeline() const;
    void CreateDescriptors();

    Pipeline pipeline;
    std::vector<VkDescriptorSet> descriptors;
};
