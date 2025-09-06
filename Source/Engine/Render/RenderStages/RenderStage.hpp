#pragma once

#include "Engine/Scene/Scene.hpp"
#include "Engine/Render/Vulkan/Frame.hpp"
#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

class RenderStage
{
public:
    RenderStage(const VulkanContext& vulkanContext, RenderContext& renderContext);
    virtual ~RenderStage();
    
    RenderStage(const RenderStage&) = delete;
    RenderStage& operator=(const RenderStage&) = delete;

    RenderStage(RenderStage&&) = delete;
    RenderStage& operator=(RenderStage&&) = delete;
    
    virtual void Prepare(const Scene& scene);
    
    virtual void Execute(const Frame& frame);
    
    virtual void RecreatePipelinesAndDescriptors();
    
    virtual void OnSceneClose();
    
    bool TryReloadShaders();
    void ApplyReloadedShaders();
    
protected:
    struct ShaderInfo
    {
        std::string_view path;
        VkShaderStageFlagBits shaderStage;
        std::function<std::span<const ShaderDefine>()> definesGetter;
    };

    void CompileShaders();
    
    void AddShaderInfo(ShaderInfo shaderInfo);
    const ShaderModule* GetShader(std::string_view path) const;
    
    std::span<const ShaderDefine> GetGlobalDefines() const
    {
        return renderContext ? renderContext->globalDefines : std::span<const ShaderDefine>();
    }
    
    const VulkanContext* vulkanContext = nullptr;
    const RenderContext* renderContext = nullptr;
    
private:
    std::unordered_map<std::string_view, ShaderModule> CompileShadersImpl();
    
    std::vector<ShaderInfo> shaderInfos;
    
    std::unordered_map<std::string_view, ShaderModule> shaders;
    std::unordered_map<std::string_view, ShaderModule> reloadedShaders;
};
