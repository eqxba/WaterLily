#pragma once

#include "Engine/Render/RenderStages/RenderStage.hpp"
#include "Engine/Render/Vulkan/Pipelines/Pipeline.hpp"

class ForwardStage : public RenderStage
{
public:
    ForwardStage(const VulkanContext& vulkanContext, RenderContext& renderContext);
    ~ForwardStage() override;
    
    void OnSceneOpen(const Scene& scene) override;
    void OnSceneClose() override;
    
    void Execute(const Frame& frame) override;
    
    void RebuildDescriptors() override;
    
private:
    Pipeline BuildMeshPipeline();
    Pipeline BuildVertexPipeline();
    void BuildDescriptors();
    
    void ExecuteMesh(const Frame& frame) const;
    void ExecuteVertex(const Frame& frame) const;
    
    std::unordered_map<GraphicsPipelineType, Pipeline> graphicsPipelines;
    std::unordered_map<GraphicsPipelineType, std::vector<VkDescriptorSet>> descriptors;
};
