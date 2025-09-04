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
    
    bool TryReloadShaders() override;
    void RecreatePipelinesAndDescriptors() override;
    
    void OnSceneClose() override;
    
private:
    Pipeline CreateBoundingSpherePipeline(const std::vector<ShaderModule>& shaderModules);
    void CreateBoundingSphereDescriptors();
    
    std::vector<VkDescriptorSet> boundingSphereDescriptors;
    
    std::vector<ShaderModule> boundingSphereShaders;
    std::vector<ShaderModule> reloadedBoundingSphereShaders;
    
    Pipeline boundingSpherePipeline;
    
    Buffer unitSphereVertexBuffer;
    Buffer unitSphereIndexBuffer;
    uint32_t unitSphereIndexCount = 0;
};
