#pragma once

#include "Engine/Render/RenderStages/RenderStage.hpp"
#include "Engine/Render/Vulkan/Pipelines/Pipeline.hpp"

class DebugStage : public RenderStage
{
public:
    DebugStage(const VulkanContext& vulkanContext, RenderContext& renderContext);
    ~DebugStage() override;
    
    void Prepare(const Scene& scene) override;
    
    void Execute(const Frame& frame) override;
    
    void RecreatePipelinesAndDescriptors() override;
    
    void OnSceneClose() override;
    
private:
    void ExecuteBoundingSpheres(const Frame& frame);
    void ExecuteFrozenFrustum(const Frame& frame);
    
    Pipeline CreateBoundingSpherePipeline();
    void CreateBoundingSphereDescriptors();
    
    Pipeline CreateLinePipeline();
    
    Pipeline boundingSpherePipeline;
    std::vector<VkDescriptorSet> boundingSphereDescriptors;
    
    Pipeline linePipeline;
    
    Buffer unitSphereVertexBuffer;
    Buffer unitSphereIndexBuffer;
    uint32_t unitSphereIndexCount = 0;
    
    Buffer frustumVertexBuffer;
    Buffer frustumIndexBuffer;
    bool updatedFrustumVertices = false;
};
