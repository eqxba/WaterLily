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

    bool TryReloadShaders() override;
    void ApplyReloadedShaders() override;
    
    void OnSceneClose() override;

private:
    Pipeline CreatePipeline(ShaderModule&& shaderModule) const;
    void CreateDescriptors();

    std::vector<VkDescriptorSet> descriptors;
    Pipeline pipeline;
    
    std::unique_ptr<ShaderModule> reloadedShader;
};
