#pragma once

#include "Engine/Render/RenderStages/RenderStage.hpp"
#include "Engine/Render/Vulkan/Pipelines/Pipeline.hpp"

class PrimitiveCullStage : public RenderStage
{
public:
    PrimitiveCullStage(const VulkanContext& vulkanContext, RenderContext& renderContext);
    ~PrimitiveCullStage() override;

    void CreateRenderTargetDependentResources() override;
    void DestroyRenderTargetDependentResources() override;
    
    void OnSceneOpen(const Scene& scene) override;
    void OnSceneClose() override;

    void Execute(const Frame& frame) override;

    void RebuildDescriptors() override;
    
    void ExecuteFirstPass(const Frame& frame);
    void BuildDepthPyramid(const Frame& frame);
    void ExecuteSecondPass(const Frame& frame);
    
    void VisualizeDepth(const Frame& frame);

private:
    Pipeline BuildFirstPassPipeline() const;
    void BuildFirstPassDescriptors();
    
    void CreateDepthPyramidRenderTargetAndSampler();
    Pipeline BuildDepthPyramidPipeline() const;
    void BuildDepthPyramidDescriptors();
    
    Pipeline BuildSecondPassPipeline() const;
    void BuildSecondPassDescriptors();

    Pipeline firstPassPipeline;
    std::vector<VkDescriptorSet> firstPassDescriptors;
    
    RenderTarget depthPyramidRenderTarget;
    Sampler depthPyramidSampler;
    Pipeline depthPyramidPipeline;
    std::vector<VkDescriptorSet> depthPyramidReductionDescriptors;
    VkDescriptorSet depthPyramidDescriptor = VK_NULL_HANDLE;
    
    Pipeline secondPassPipeline;
    std::vector<VkDescriptorSet> secondPassDescriptors;
};
