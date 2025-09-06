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
    Pipeline CreateBoundingSpherePipeline();
    void CreateBoundingSphereDescriptors();
    
    Pipeline boundingSpherePipeline;
    std::vector<VkDescriptorSet> boundingSphereDescriptors;
    
    Buffer unitSphereVertexBuffer;
    Buffer unitSphereIndexBuffer;
    uint32_t unitSphereIndexCount = 0;
};
