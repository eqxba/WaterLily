#pragma once

#include "Engine/Render/RenderStages/RenderStage.hpp"
#include "Engine/Render/Vulkan/Pipelines/Pipeline.hpp"

class ForwardStage : public RenderStage
{
public:
    ForwardStage(const VulkanContext& vulkanContext, RenderContext& renderContext);
    ~ForwardStage() override;
    
    void Prepare(const Scene& scene) override;
    
    void Execute(const Frame& frame) override;
    
    void RecreatePipelinesAndDescriptors() override;
    
    void OnSceneClose() override;
    
private:
    Pipeline CreateMeshPipeline();
    Pipeline CreateVertexPipeline();
    void CreateDescriptors();
    
    void ExecuteMesh(const Frame& frame) const;
    void ExecuteVertex(const Frame& frame) const;
    
    std::unordered_map<GraphicsPipelineType, Pipeline> graphicsPipelines;
    std::unordered_map<GraphicsPipelineType, std::vector<VkDescriptorSet>> descriptors;
};
