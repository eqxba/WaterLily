#pragma once

#include "Engine/Render/RenderOptions.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/RenderStages/RenderStage.hpp"
#include "Engine/Render/Vulkan/Pipelines/Pipeline.hpp"

class ForwardStage : public RenderStage
{
public:
    ForwardStage(const VulkanContext& vulkanContext, RenderContext& renderContext);
    ~ForwardStage() override;
    
    void Prepare(const Scene& scene) override;
    
    void Execute(const Frame& frame) override;
    
    void RecreateFramebuffers() override;
    
    bool TryReloadShaders() override;
    void ApplyReloadedShaders() override;
    
    void OnSceneClose() override;
    
private:
    Pipeline CreateMeshPipeline(const std::vector<ShaderModule>& shaderModules);
    Pipeline CreateVertexPipeline(const std::vector<ShaderModule>& shaderModules);
    void CreateDescriptors();
    
    void ExecuteMesh(const Frame& frame) const;
    void ExecuteVertex(const Frame& frame) const;
    
    RenderPass renderPass;
    std::vector<VkFramebuffer> framebuffers;
    
    std::unordered_map<GraphicsPipelineType, std::vector<VkDescriptorSet>> descriptors;
    
    std::unordered_map<GraphicsPipelineType, std::vector<ShaderModule>> shaders;
    std::unordered_map<GraphicsPipelineType, std::vector<ShaderModule>> reloadedShaders;
    
    std::unordered_map<GraphicsPipelineType, Pipeline> graphicsPipelines;
};
