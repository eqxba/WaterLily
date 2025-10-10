#pragma once

#include "Engine/Scene/Scene.hpp"
#include "Engine/Render/Vulkan/Frame.hpp"
#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

class RenderStage
{
public:
    RenderStage(const VulkanContext& vulkanContext, const RenderContext& renderContext);
    virtual ~RenderStage();
    
    RenderStage(const RenderStage&) = delete;
    RenderStage& operator=(const RenderStage&) = delete;

    RenderStage(RenderStage&&) = delete;
    RenderStage& operator=(RenderStage&&) = delete;
    
    virtual void CreateRenderTargetDependentResources();
    virtual void DestroyRenderTargetDependentResources();
    
    virtual void OnSceneOpen(const Scene& scene);
    virtual void OnSceneClose();
    
    virtual void Execute(const Frame& frame);
    
    virtual void RebuildDescriptors();
    
    bool TryRebuildPipelines();
    void ApplyRebuiltPipelines();
    
protected:
    using PipelineBuildFunction = std::function<Pipeline()>;
    
    void AddPipeline(Pipeline& pipelineReference, PipelineBuildFunction buildFunction);
    
    ShaderModule GetShader(std::string_view path, VkShaderStageFlagBits shaderStage,
        std::span<std::string_view> runtimeDefines, std::span<const ShaderDefine> defines) const;
    
    const VulkanContext* const vulkanContext = nullptr;
    const RenderContext* const renderContext = nullptr;
    
private:
    std::vector<std::pair<Pipeline*, PipelineBuildFunction>> pipelineData;
    std::vector<Pipeline> rebuiltPipelines;
};
